/* Thomas BERNARD */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "hfscommon.h"

int main(int argc, char * * argv)
{
	FILE * f;

	if(argc < 2) {
		fprintf(stderr, "usage: %s <file.iso>\n", argv[0]);
		return 1;
	}
	f = fopen(argv[1], "rb");
	if(!f) {
		fprintf(stderr, "Failed to open file %s\n", argv[1]);
		return 2;
	}
	read_partition_map(f);
	if(hfs_part_sector == 0 && hfs_part_sector_count == 0) {
		printf("HFS/HFS+ partition not found\n");
	} else {
		read_hfs_volume_header(f);
		read_catalog(f);
	}
	fclose(f);
	return 0;
}

