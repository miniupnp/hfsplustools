/* Thomas BERNARD */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hfscommon.h"

enum command {
	INVALID = 0,
	SHOW = 1,
	DISABLE,
	SETROOT,
	SETID
};

void writeu32(unsigned char * p, uint32_t i)
{
	p[0] = (i >> 24) & 0xff;
	p[1] = (i >> 16) & 0xff;
	p[2] = (i >> 8) & 0xff;
	p[3] = i & 0xff;
}

#define AUTO_OPEN_DIR_ID_OFFSET (80 + 2 * 4)
void set_auto_folder(FILE * f, uint32_t folderid)
{
	unsigned char buffer[512];
	unsigned char a_buffer[512];
	uint32_t dir_id;
	int write_alternate = 0;

	if(fseek(f, hfs_part_sector * SECTOR_SIZE + HFS_VOLUME_HEADER_OFFSET, SEEK_SET) != 0) {
		perror("fseek");
		return;
	}
	if(fread(buffer, 1, sizeof(buffer), f) != sizeof(buffer)) {
		fprintf(stderr, "reading error\n");
		return;
	}
	printf("SIGNATURE %c%c\n", buffer[0], buffer[1]);
	if(buffer[0] != 'H' || buffer[1] != '+') {
		printf("Not HFS+\n");
		return;
	}
	if(fseek(f, (hfs_part_sector + hfs_part_sector_count) * SECTOR_SIZE - HFS_VOLUME_HEADER_OFFSET, SEEK_SET) != 0) {
		perror("fseek");
		return;
	}
	if(fread(a_buffer, 1, sizeof(a_buffer), f) != sizeof(a_buffer)) {
		fprintf(stderr, "reading error\n");
		return;
	}
	if(memcmp(buffer, a_buffer, sizeof(buffer)) == 0) {
		write_alternate = 1;
	}
	dir_id = readu32(buffer + AUTO_OPEN_DIR_ID_OFFSET);
	printf("Former value = %u\n", dir_id);
	if(dir_id == folderid) {
		printf("No need to patch\n");
		return;
	}
	writeu32(buffer + AUTO_OPEN_DIR_ID_OFFSET, folderid);
	dir_id = readu32(buffer + AUTO_OPEN_DIR_ID_OFFSET);
	printf("New value = %u\n", dir_id);

	/* Main Volume Header */
	if(fseek(f, hfs_part_sector * SECTOR_SIZE + HFS_VOLUME_HEADER_OFFSET, SEEK_SET) != 0) {
		perror("fseek");
		return;
	}
	if(fwrite(buffer, 1, sizeof(buffer), f) != sizeof(buffer)) {
		fprintf(stderr, "write error\n");
		return;
	}

	/* Alternate Volume Header */
	if(write_alternate) {
		if(fseek(f, (hfs_part_sector + hfs_part_sector_count) * SECTOR_SIZE - HFS_VOLUME_HEADER_OFFSET, SEEK_SET) != 0) {
			perror("fseek");
			return;
		}
		if(fwrite(buffer, 1, sizeof(buffer), f) != sizeof(buffer)) {
			fprintf(stderr, "write error\n");
			return;
		}
	}

	printf("successfully patched !\n");
}

void usage(const char * progname)
{
	fprintf(stderr, "usage: %s <command> <file.iso>\n", progname);
	fprintf(stderr, "commands :\n");
	fprintf(stderr, "  show : only display informations\n");
	fprintf(stderr, "  disable : disable autofolder\n");
	fprintf(stderr, "  setroot : set autofolder to root\n");
	fprintf(stderr, "  setfolderid <id> : set autofolder to id * USE WITH CARE *\n");
}

int main(int argc, char * * argv)
{
	FILE * f;
	int ok = 0;
	const char * filename = NULL;
	enum command cmd = INVALID;
	uint32_t folderid = 0;

	if(argc < 3 || 0 == strcmp(argv[1], "help")) {
		usage(argv[0]);
		return 1;
	}
	filename = argv[2];
	if(strcmp(argv[1], "show") == 0) {
		cmd = SHOW;
	} else if(strcmp(argv[1], "disable") == 0) {
		cmd = DISABLE;
		folderid = 0;
	} else if(strcmp(argv[1], "setroot") == 0) {
		cmd = SETROOT;
		folderid = 2; /* ROOT folder */
	} else if(strcmp(argv[1], "setfolderid") == 0) {
		folderid = strtoul(argv[2], NULL, 0);
		if(argc < 4) {
			usage(argv[0]);
			return 1;
		}
		filename = argv[3];
	} else {
		fprintf(stderr, "unknown command '%s'\n\n", argv[1]);
		usage(argv[0]);
		return 1;
	}
	f = fopen(filename, "rb");
	if(!f) {
		fprintf(stderr, "Failed to open file %s\n", filename);
		return 2;
	}
	read_partition_map(f);
	if(hfs_part_sector == 0 && hfs_part_sector_count == 0) {
		printf("HFS/HFS+ partition not found\n");
	} else {
		ok = read_hfs_volume_header(f);
	}
	fclose(f);

	if(ok) {
		switch(cmd) {
		case DISABLE:
		case SETROOT:
		case SETID:
			f = fopen(filename, "r+b");
			if(!f) {
				fprintf(stderr, "Failed to open file %s for Writing\n", filename);
				return 4;
			}
			set_auto_folder(f, folderid);
			fclose(f);
			break;
		case SHOW:
			(void)0;
			break;
		default:
			fprintf(stderr, "*** INVALID COMMAND ***\n");
			return 5;
		}
	} else {
		fprintf(stderr, "Errors reading HFS+ volume from %s\n", filename);
		return 3;
	}

	return 0;
}

