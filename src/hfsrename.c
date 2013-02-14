/* Thomas BERNARD */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hfscommon.h"

int main(int argc, char * * argv)
{
	FILE * f;
	const char * isofile = NULL;
	const char * newname = NULL;
	int i;
	unsigned char * catalog = NULL;
	struct find_infos infos;

	for(i = 1; i < argc; i++) {
		if(isofile == NULL)
			isofile = argv[i];
		else if(newname == NULL)
			newname = argv[i];
		else {
			fprintf(stderr, "unrecognized argument : %s\n", argv[i]);
			return 1;
		}
	}
	if(isofile == NULL) {
		fprintf(stderr, "Usage: %s [] <image.iso> [new volume name]\n", argv[0]);
		return 1;
	}
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
		catalog = load_catalog(f);
		if(!print_node(catalog)) { /* header node */
			free(catalog);
			return 4;
		}
		/* root node */
		printf("\nroot node :\n");
		print_node(catalog + catalog_root_node * catalog_node_size);
		if(hfs_find(catalog, 1, NULL, &infos)) {
			printf("Root Folder found : id=%u\n", infos.folder_id);
			//
		}
	}
	fclose(f);
	free(catalog);
	printf("\n! Warning : Does nothing !\n");
	/* this program is not finished ! */
	return 0;
}

