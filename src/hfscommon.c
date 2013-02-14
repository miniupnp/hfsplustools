/* Thomas BERNARD */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include "hfscommon.h"

uint32_t hfs_part_sector;
uint32_t hfs_part_sector_count;
uint32_t block_size;
uint32_t catalog_start_block;
uint32_t catalog_block_count;
uint32_t catalog_node_size;
uint32_t catalog_root_node;

void hexdump(const unsigned char * p, unsigned int n, unsigned int a)
{
	unsigned int i, j;

	while (n > 0) {
		i = (n >= 16) ? 16 : n;
		printf("%08x: ", a);
		for (j = 0; j < i; j++) {
			printf("%02x ", (unsigned)p[j]);
		}
		for (; j < 16; j++)
			printf("   ");
		printf ("| ");
		for (j = 0; j < i; j++) {
			if (isgraph(p[j]))
				putchar(p[j]);
			else
				putchar(' ');
		}
		putchar('\n');
		n -= i;
		p += i;
		a += i;
	}
}

uint16_t readu16(const unsigned char * p)
{
	return p[0] << 8 | p[1];
}

uint32_t readu32(const unsigned char * p)
{
	return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

/* http://www.informit.com/articles/article.aspx?p=376123&seqNum=3 */
/* http://www.dubeyko.com/development/FileSystems/HFSPLUS/hexdumps/hfsplus_volume_header.html */
int read_hfs_volume_header (FILE * f)
{
	unsigned char buffer[512];
	unsigned char a_buffer[512];
	uint32_t attributes;
	int i;
	uint32_t bsize;
	uint32_t bcount;

	if(fseek(f, hfs_part_sector * SECTOR_SIZE + HFS_VOLUME_HEADER_OFFSET, SEEK_SET) != 0) {
		perror("fseek");
		return 0;
	}
	if(fread(buffer, 1, sizeof(buffer), f) != sizeof(buffer)) {
		fprintf(stderr, "reading error\n");
		return 0;
	}
	printf("VOLUME HEADER (offset %08x)\n",
		hfs_part_sector * SECTOR_SIZE + HFS_VOLUME_HEADER_OFFSET);
	printf("SIGNATURE %c%c version %02x%02x (4=HFS+ 5=HFSX)\n",
	       buffer[0], buffer[1], buffer[2], buffer[3]);
	if(buffer[0] != 'H' || buffer[1] != '+') {
		printf("Not HFS+\n");
		return 0;
	}
	if(fseek(f, (hfs_part_sector + hfs_part_sector_count) * SECTOR_SIZE - HFS_VOLUME_HEADER_OFFSET, SEEK_SET) != 0) {
		perror("fseek");
		return 0;
	}
	if(fread(a_buffer, 1, sizeof(a_buffer), f) != sizeof(a_buffer)) {
		fprintf(stderr, "reading error\n");
		return 0;
	}
	printf("ALTERNATE VOLUME HEADER (offset %08x)\n",
		(hfs_part_sector + hfs_part_sector_count) * SECTOR_SIZE - HFS_VOLUME_HEADER_OFFSET);
	if(memcmp(buffer, a_buffer, sizeof(buffer)) != 0) {
		printf("WARNING: main and alternate Volume Headers differ\n");
		if(memcmp(buffer, a_buffer, 20) != 0) {
			fprintf(stderr, "ERROR: first 20 bytes of main and alternate Volume Headers differ\n");
			printf("Main:\n");
			hexdump(buffer, 32, 0);
			printf("Alternate:\n");
			hexdump(a_buffer, 32, 0);
			return 0;
		}
	}
	hexdump(buffer, 128, 0);
	attributes = readu32(buffer + 4);
	printf("attributes = %08x ", attributes);
	if(attributes&(1<<8)) printf("|kHFSVolumeUnmountedBit");
	if(attributes&(1<<9)) printf("|kHFSVolumeSparedBlocksBit");
	if(attributes&(1<<10)) printf("|kHFSVolumeNoCacheRequiredBit");
	if(attributes&(1<<11)) printf("|kHFSBootVolumeInconsistentBit");
	if(attributes&(1<<12)) printf("|kHFSCatalogNodeIDsReusedBit");
	if(attributes&(1<<13)) printf("|kHFSVolumeJournaledBit");
	if(attributes&(1<<15)) printf("|kHFSVolumeSoftwareLockBit");
	printf("\nlast mounted version '%c%c%c%c', ", buffer[8], buffer[9], buffer[10], buffer[11]);
	printf("journalInfoBlock %08x\n", readu32(buffer + 12));
	printf("createDate %08x, ", readu32(buffer + 16));
	printf("modifyDate %08x\n", readu32(buffer + 20));
	printf("backupDate %08x, ", readu32(buffer + 24));
	printf("checkedDate %08x\n", readu32(buffer + 28));
	printf("file count %u, ", readu32(buffer + 32));
	printf("folder count %u\n", readu32(buffer + 36));
	bsize = readu32(buffer + 40);
	block_size = bsize;
	bcount = readu32(buffer + 44);
	printf("block size %u, ", bsize);
	printf("total blocks %u (total %u bytes)\n",
		bcount, bsize * bcount);
	if(bsize * bcount != hfs_part_sector_count * SECTOR_SIZE)
		fprintf(stderr, "*** Partion size inconsistency with Partition Map ***\n");
	printf("free blocks %u, ", readu32(buffer + 48));
	printf("next catalog id %u\n", readu32(buffer + 64));
	for(i = 0; i < 8; i++) {
		printf("finderInfo[%d] = %08x", i, readu32(buffer + 80 + i * 4));
		if(i==2) printf(" directory ID auto Finder display");
		putchar('\n');
	}
	/* 80 + 8*4 = 112 */
	/* struct HFSPlusForkData {
    UInt64                  logicalSize;
    UInt32                  clumpSize;
    UInt32                  totalBlocks;
    HFSPlusExtentRecord     extents;
};
	8 + 4 + 4 + 8 * 8 = 16 + 64 = 80
	typedef HFSPlusExtentDescriptor HFSPlusExtentRecord[8];
	struct HFSPlusExtentDescriptor {
    UInt32                  startBlock;
    UInt32                  blockCount;
};
 */
	/* HFSPlusForkData     allocationFile;
    HFSPlusForkData     extentsFile;
    HFSPlusForkData     catalogFile;
    HFSPlusForkData     attributesFile;
    HFSPlusForkData     startupFile;*/
	printf("catalogFile:\n");
	//hexdump(buffer+112+80*2, 80, 0);
	catalog_start_block = readu32(buffer+112+80*2+16);
	catalog_block_count = readu32(buffer+112+80*2+20);
	printf("  startBlock = %08x (%u)\n", catalog_start_block, catalog_start_block);
	printf("  blockCount = %08x (%u)\n", catalog_block_count, catalog_block_count);
	return 1;
}

static void print_hfs_uni(const unsigned char * p)
{
	uint16_t len;
	len = readu16(p);
	p += 2;
	putchar('"');
	while(len > 0) {
		putchar(p[1]);
		p += 2;
		len--;
	}
	putchar('"');
}

static int hfs_uni_to_str(char * d, int n, const unsigned char * p)
{
	uint16_t len;
	int i;
	len = readu16(p);
	p += 2;
	for(i = 0; i < len && i < (n - 1); i++) {
		d[i] = p[1];
		p += 2;
	}
	d[i] = '\0';
	return i;
}

/*
int parse_node(const unsigned char * p)
{
}
*/
/* should be replaced by parse */
int print_node(const unsigned char * p)
{
	uint16_t num_records;
	uint16_t rec;
	uint8_t kind;
	const unsigned char * q;
	/* node descriptor */
	printf("fLink=%u ", readu32(p));
	printf("bLink=%u ", readu32(p+4));
	kind = p[8];
	printf("kind=%u ", kind);
	 /* kBTLeafNode       = -1,
    kBTIndexNode      =  0,
    kBTHeaderNode     =  1,
    kBTMapNode        =  2 */
	printf("height=%u ", p[9]);
	num_records = readu16(p+10);
	printf("numRecords=%u\n", num_records);
	q = p + 14;
	if(kind == 1) { /* header node */
		unsigned totalNodes;
		catalog_root_node = readu32(q + 2);
		printf("rootNode = %u\n", catalog_root_node);
		printf("leafRecords = %u\n", readu32(q + 6));
		printf("firstLeafNode = %u\n", readu32(q + 10));
		printf("lastLeafNode = %u\n", readu32(q + 14));
		catalog_node_size = readu16(q + 18);
		totalNodes = readu32(q + 22);
		printf("nodeSize=%u\n", catalog_node_size);
		printf("maxKeyLength=%u\n", readu16(q + 20));
		printf("totalNodes=%u (%u bytes)\n",
		       totalNodes, totalNodes * catalog_node_size);
		if(catalog_block_count * block_size != totalNodes * catalog_node_size) {
			fprintf(stderr, "ERROR catalog size inconsistency !\n");
			return 0;
		}
		printf("freeNodes=%u\n", readu32(q + 26));
		printf("clumpSize=%u\n", readu32(q + 32));
		printf("btreeType=%u\n", q[36]);
		printf("keyCompareType=%u\n", q[37]);
		printf("attributes %08X\n", readu32(q + 38));
	} else if(kind == 0) {	/* index node */
		for(rec = 0; rec < num_records; rec++) {
			/* record */
			uint16_t key_length = readu16(q);
			uint32_t node_number = readu32(q + key_length + 2);
			printf("%3u ", rec);
			printf("length=%u\n", key_length);
			//hexdump(q + 2, key_length, 0);
			printf("    ");
			print_hfs_uni(q + 6);
			printf(" parentId=%u ", readu32(q + 2));
			printf("=> %u\n", node_number);
			q += key_length + 2 + 4;
		}
	} else {
		printf("unprocessed kind %u\n", kind);
	}
	return 1;
}

static
int hfs_find_in_node(unsigned char * catalog, unsigned char * p,
                     uint32_t parent_id, const char * name,
                     struct find_infos * infos)
{
	uint8_t kind;
	uint16_t rec, num_records;
	uint32_t previous_node_number = 0;
	kind = p[8];
	num_records = readu16(p+10);
	p += 14;
	switch(kind) {
	case 0:	/* index */
		for(rec = 0; rec < num_records; rec++) {
			/* record */
			char str[256];
			uint16_t key_length = readu16(p);
			uint32_t cur_parent_id = readu32(p + 2);
			uint32_t node_number = readu32(p + key_length + 2);
			hfs_uni_to_str(str, sizeof(str), p + 6);
			if(parent_id == cur_parent_id) {
				if((name == NULL) || (0==strcasecmp(str, name))) {
					/* this one */
					p = catalog + node_number * catalog_node_size;
					return hfs_find_in_node(catalog, p, parent_id, name, infos);
				} else if(strcasecmp(str, name) > 0) {
					/* previous one ! */
					p = catalog + previous_node_number * catalog_node_size;
					return hfs_find_in_node(catalog, p, parent_id, name, infos);
				}
			} else if(cur_parent_id > parent_id) {
				/* previous one ! */
				p = catalog + previous_node_number * catalog_node_size;
				return hfs_find_in_node(catalog, p, parent_id, name, infos);
			}
			p += key_length + 2 + 4;
			previous_node_number = node_number;
		}
		printf("OUT\n");
		break;
	case 255:	/* LEAF */
		printf("LEAF %u records\n", num_records);
		for(rec = 0; rec < num_records; rec++) {
			char str[256];
			uint16_t record_type;
			uint16_t key_length = readu16(p);
			unsigned char * q;
			uint32_t valence;
			uint32_t folder_id;
			uint32_t cur_parent_id = readu32(p + 2);
			uint16_t HFSflags;
			hfs_uni_to_str(str, sizeof(str), p + 6);
			q = p + 2 + key_length;
			record_type = readu16(q);
			printf("%3d record type=%u parentId=%u \"%s\"",
			       rec, record_type, cur_parent_id, str);
			putchar('\n');
			//hexdump(q + 2, 32, 0);
			switch(record_type) {
			case 1: /* folder record */
				/* flags = readu16(q + 2); */
				valence = readu32(q + 4);
				folder_id = readu32(q + 8);
				HFSflags = readu16(q+56);
				//UInt32              createDate;
				//UInt32              contentModDate;
				//UInt32              attributeModDate;
				//UInt32              accessDate;
				//UInt32              backupDate;
				//HFSPlusBSDInfo      permissions;
				//FolderInfo          userInfo;
				//ExtendedFolderInfo  finderInfo;
				//UInt32              textEncoding;
				//UInt32              reserved;
				printf("    valence=%u folder_id=%u\n", valence, folder_id);
				printf("    folderinfo : (%u,%u)-(%u,%u) %04X",
					readu16(q+48), readu16(q+50),
					readu16(q+52), readu16(q+54),
					HFSflags);
				if(HFSflags & 0x0400) printf(" Custom Icon");
				if(HFSflags & 0x4000) printf(" Invisible");
				putchar('\n');
				if(cur_parent_id == parent_id &&
				   (name == NULL || 0 == strcmp(name, str))) {
					printf("FOUND FOLDER\n");
					infos->folder_id = folder_id;
					infos->p = q;
					return 1;
				}
				p = q + 88;
				break;
			case 2: /* file record */
				p = q + 248;
				break;
			case 3:	/* folder thread record */
			case 4:	/* file thread record */
				{
					uint16_t len;
					cur_parent_id = readu32(q + 4);
					len = readu16(q + 8);
					printf("parent_id=%u len=%u\n", cur_parent_id, len);
					p = q + 10 + len * 2;
				}
				break;
			default:
				printf("unknown\n");
				return 0;
			}
		}
		break;
	default:
		printf("unrecognized kind %u\n", kind);
	}
	return 0;
}

int hfs_find(unsigned char * catalog, uint32_t parent_id,
		  const char * name, struct find_infos * infos)
{
	unsigned char * p;

	p = catalog + catalog_root_node * catalog_node_size; /* root node */
	return hfs_find_in_node(catalog, p, parent_id, name, infos);
}


/* must be free'd after usage */
unsigned char * load_catalog(FILE * f)
{
	unsigned char * catalog;
	int offset;
	if(block_size == 0) {
		fprintf(stderr, "block_size==0 !\n");
		return 0;
	}
	if(catalog_start_block == 0) {
		fprintf(stderr, "catalog_start_block == 0 !\n");
		return 0;
	}
	if(catalog_block_count == 0) {
		fprintf(stderr, "catalog_block_count == 0 !\n");
		return 0;
	}
	offset = hfs_part_sector * SECTOR_SIZE;
	offset += block_size * catalog_start_block;
	if(fseek(f, offset, SEEK_SET) != 0) {
		perror("fseek");
		return 0;
	}
	catalog = malloc(catalog_block_count * block_size);
	if(!catalog) {
		fprintf(stderr, "memory allocation error\n");
		return 0;
	}
	if(fread(catalog, block_size, catalog_block_count, f) != catalog_block_count) {
		fprintf(stderr, "failed to read catalog file\n");
		free(catalog);
		return 0;
	}
	printf("%u bytes in catalog\n", catalog_block_count * block_size);
	return catalog;
}

int save_catalog(FILE * f, unsigned char * catalog)
{
	int offset;
	if(block_size == 0) {
		fprintf(stderr, "block_size==0 !\n");
		return 0;
	}
	if(catalog_start_block == 0) {
		fprintf(stderr, "catalog_start_block == 0 !\n");
		return 0;
	}
	if(catalog_block_count == 0) {
		fprintf(stderr, "catalog_block_count == 0 !\n");
		return 0;
	}
	offset = hfs_part_sector * SECTOR_SIZE;
	offset += block_size * catalog_start_block;
	if(fseek(f, offset, SEEK_SET) != 0) {
		perror("fseek");
		return 0;
	}
	if(fwrite(catalog, block_size, catalog_block_count, f) != catalog_block_count) {
		fprintf(stderr, "failed to write to file\n");
		return 0;
	}
	if(fflush(f) != 0) {
		perror("fflush");
		return 0;
	}
	return 1;
}

int read_catalog(FILE * f)
{
	unsigned char * catalog;
	//struct find_infos infos;

	catalog = load_catalog(f);
	if(catalog == NULL)
		return 0;
	if(!print_node(catalog)) {	/* header node */
		free(catalog);
		return 0;
	}
#if 0
	//print_node(catalog + catalog_root_node * catalog_node_size);/* root node */
	hfs_find(catalog, 1 /* root parent id */, NULL, &infos);
	printf("----\n");
	if(hfs_find(catalog, 2 /* root id */, "fichiers", &infos)) {
		printf("/fichiers found : id=%u\n", infos.folder_id);
		if(hfs_find(catalog, infos.folder_id, "MYRIADE_3E.app", &infos)) {
			printf("found ! flags=%04X\n", readu16(infos.p + 56));
			//found = 1;
		}
	}
#endif
	free(catalog);
	return 1;
}

/* http://opensource.apple.com/source/IOStorageFamily/IOStorageFamily-116/IOApplePartitionScheme.h
 * http://www.informit.com/articles/article.aspx?p=376123&seqNum=3 */
void read_partition_map(FILE * f)
{
	unsigned char buffer[SECTOR_SIZE];
	unsigned int i = 0;
	unsigned int count = 0;

	fseek(f, PARTITION_MAP_OFFSET, SEEK_SET);
	do {
		unsigned int start, size;
		unsigned int start_l, size_l;
		fread(buffer, 1, sizeof(buffer), f);
		hexdump(buffer, 136, 0);
		printf("=== partition %u ===\n", i);
		if(readu16(buffer) != 0x504D) {	// signature
			printf("bad signature\n");
			break;
		}
		count = readu32(buffer + 4);
		printf("total number of partitions = %u\n", count);
		start = readu32(buffer + 8);
		size = readu32(buffer + 12);
		printf("starting sector=%u (offset %08x) size %u (%u bytes)\n",
			start, start * SECTOR_SIZE, size, size * SECTOR_SIZE);
		start_l = readu32(buffer + 80);
		size_l = readu32(buffer + 84);
		printf("logical starting sector=%u (offset %08x) size %u (%u bytes)\n",
			start_l, start_l * SECTOR_SIZE, size_l, size_l * SECTOR_SIZE);
		printf(" name = '%s' type = '%s'\n", buffer + 16, buffer + 48);
		if(strcmp((char *)buffer + 48, "Apple_HFS") == 0) {
			printf("---\n");
			hfs_part_sector = start;
			hfs_part_sector_count = size;
		}
		i++;
	} while(i < count);
}


