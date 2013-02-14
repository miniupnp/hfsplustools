/* Thomas BERNARD */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hfscommon.h"

void writeu16(unsigned char * p, uint16_t i)
{
	p[0] = (i >> 8) & 0xff;
	p[1] = i & 0xff;
}

/* return :
	42 Not found
	43 dont need patching
*/ 
int main(int argc, char * * argv)
{
	FILE * f;
	const char * isofile = NULL;
	const char * appname = NULL;
	int i;
	int doit = 0;
	int verbose = 0;
	unsigned char * catalog = NULL;
	int found = 0;
	int need_patching = 0;
	struct find_infos infos;
	int ret = 0;

	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-v")==0)
			verbose++;
		else if(strcmp(argv[i], "--doit")==0)
			doit = 1;
		else if(isofile == NULL)
			isofile = argv[i];
		else if(appname == NULL)
			appname = argv[i];
		else {
			fprintf(stderr, "unrecognized argument : %s\n", argv[i]);
			return 1;
		}
	}
	if(isofile == NULL || appname == NULL) {
		fprintf(stderr, "Usage: %s [--doit] [-v] <image.iso> FLIPBOOK.app\n", argv[0]);
		return 1;
	}
	printf("isofile=%s appname=%s\n", isofile, appname);
	f = fopen(isofile, "rb");
	if(f == NULL) {
		fprintf(stderr, "cannot open %s for reading\n", isofile);
		return 2;
	}
	read_partition_map(f);
	if(hfs_part_sector == 0 && hfs_part_sector_count == 0) {
		printf("HFS/HFS+ partition not found\n");
		fclose(f);
		return 3;
	} else {
		read_hfs_volume_header(f);
		//read_catalog(f);
		catalog = load_catalog(f);
		if(!print_node(catalog)) { /* header node */
			free(catalog);
			return 4;
		}
		/* find fichier/appname.app */
		if(hfs_find(catalog, 2 /* root id */, "fichiers", &infos)) {
			printf("Folder /fichiers found : id=%u\n", infos.folder_id);
			if(hfs_find(catalog, infos.folder_id, appname, &infos)) {
				printf("Folder /fichier/%s found ! flags=%04X\n",
				       appname, readu16(infos.p + 56));
				found = 1;
				if(readu16(infos.p + 56) & 0x4000)
					need_patching = 1;
			}
		}
	}
	fclose(f);
	if(!found) {
		fprintf(stderr, "Folder /fichiers/%s NOT FOUND !\n", appname);
		ret = 42;
	} else if(!need_patching) {
		fprintf(stderr, "%s doesn't need patching\n", isofile);
		ret = 43;
	}
	//   kIsInvisible    = 0x4000,     /* Files and folders */
	if(doit && found && need_patching) {
		uint16_t HFSflags, newHFSflags;
		printf("unhidding fichiers/%s\n", appname);
		HFSflags = readu16(infos.p + 56);
		newHFSflags = HFSflags & ~0x4000;
		printf("flags %04X => %04X (offset %Xh in catalog)\n",
		       HFSflags, newHFSflags, (unsigned)(infos.p + 56 - catalog));
		writeu16(infos.p + 56, newHFSflags);
		printf("writing patched catalog\n");
		f = fopen(isofile, "r+b");
		if(f == NULL) {
			fprintf(stderr, "cannot open %s for writing\n", isofile);
			ret = 5;
		} else {
			if(save_catalog(f, catalog)) {
				printf("SUCCESS\n");
				ret = 0;
			} else {
				fprintf(stderr, "save_catalog() failed\n");
				ret = 6;
			}
		}
		fclose(f);
	}
	free(catalog);
	return ret;
}

