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
	const char * path = NULL;
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
		else if(path == NULL)
			path = argv[i];
		else {
			fprintf(stderr, "unrecognized argument : %s\n", argv[i]);
			return 1;
		}
	}
	if(isofile == NULL || path == NULL) {
		fprintf(stderr, "Usage: %s [--doit] [-v] <image.iso> path/.../file.txt\n", argv[0]);
		return 1;
	}
	printf("isofile=%s path=%s\n", isofile, path);
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
		const char * curpath;
		char pathelt[256];
		uint32_t folder_id;

		read_hfs_volume_header(f);
		//read_catalog(f);
		catalog = load_catalog(f);
		if(!print_node(catalog)) { /* header node */
			free(catalog);
			return 4;
		}
		/* find path/.../file.txt */
		curpath = path;
		folder_id = 2; /* root id */
		do {
			char * p;
			size_t len;
			p = strchr(curpath, '/');
			if(p) {
				len = p - curpath;
				if(len >= sizeof(pathelt))
					len = sizeof(pathelt) - 1;
				memcpy(pathelt, curpath, len);
				pathelt[len] = '\0';
				curpath = p + 1;
			} else {
				strncpy(pathelt, curpath, sizeof(pathelt));
				curpath = NULL;
			}
			if(!hfs_find(catalog, folder_id, pathelt, &infos)) {
				/* not found */
				break;
			}
			if(curpath) {
				folder_id = infos.folder_id;
				printf("Folder %s found : id=%u\n", pathelt, folder_id);
			} else {
				printf("Folder/file %s found ! flags=%04X\n",
				       path, readu16(infos.p + 56));
				found = 1;
				if(readu16(infos.p + 56) & 0x4000)
					need_patching = 1;
			}
		} while(curpath && !found);
	}
	fclose(f);
	if(!found) {
		fprintf(stderr, "Folder/file %s NOT FOUND !\n", path);
		ret = 42;
	} else if(!need_patching) {
		fprintf(stderr, "%s doesn't need patching\n", isofile);
		ret = 43;
	}
	//   kIsInvisible    = 0x4000,     /* Files and folders */
	if(doit && found && need_patching) {
		uint16_t HFSflags, newHFSflags;
		printf("unhidding %s\n", path);
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

