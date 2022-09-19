#ifndef DISK_H
#define DISK_H

#include "fs/file.h"

typedef unsigned int PEACHOS_DISK_TYPE;

#define ATA_COMMAND_PORT 0x1F7
// Port to send drive and bit 24 - 27 of LBA
#define ATA_LBA_UPPER_BITS_PORT 0x01F6
// Port to send bit 16 - 23 of LBA
#define ATA_LBA_HIGH_BITS_PORT 0x1F5
// Port to send bit 8 - 15 of LBA
#define ATA_LBA_MIDDLE_BITS_PORT 0x1F4
// Port to send bit 0 - 7 of LBA
#define ATA_LBA_LOW_BITS_PORT 0x1F3
// Port to send number of sectors
#define ATA_NUMBER_OF_SECTORS_PORT 0x01F2
#define ATA_DATA_IN_OUT_PORT 0x1F0
#define ATA_COMMAND_READ_WITH_RETRY 0x20
// Set bit 6 for LBA mode
#define ATA_LBA_MODE 0b11100000
#define ATA_SECTOR_BUFFER_REQUIRES_SERVICING 0x08

// Represents a real physical hard disk
#define PEACHOS_DISK_TYPE_REAL 0

struct disk
{
    PEACHOS_DISK_TYPE type;
    int sector_size;

    // The id of the disk
    int id;

    struct filesystem *filesystem;

    // The private data of our filesystem
    void *fs_private;
};

void disk_search_and_init();
struct disk *disk_get(int index);
int disk_read_block(struct disk *idisk, unsigned int lba, int total, void *buf);

#endif