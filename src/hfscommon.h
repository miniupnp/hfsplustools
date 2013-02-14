/* Thomas BERNARD */
#ifndef __HFSCOMMON_H__
#define __HFSCOMMON_H__

#include <stdint.h>
#include <stdio.h>

#define SECTOR_SIZE 512
#define PARTITION_MAP_OFFSET 512
#define HFS_VOLUME_HEADER_OFFSET 1024

struct find_infos {
	unsigned char * p;	/* struct HFSPlusCatalogFolder or HFSPlusCatalogFile */
	uint32_t folder_id;
};


extern uint32_t hfs_part_sector;
extern uint32_t hfs_part_sector_count;

extern uint32_t catalog_node_size;
extern uint32_t catalog_root_node;

uint16_t readu16(const unsigned char * p);

uint32_t readu32(const unsigned char * p);

int read_hfs_volume_header (FILE * f);

unsigned char * load_catalog(FILE * f);

int save_catalog(FILE * f, unsigned char * catalog);

int read_catalog(FILE * f);

int print_node(const unsigned char * p);

int hfs_find(unsigned char * catalog, uint32_t parent_id,
		  const char * name, struct find_infos * infos);

void read_partition_map(FILE * f);

#endif

