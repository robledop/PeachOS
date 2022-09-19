#include "disk.h"
#include "io/io.h"
#include "config.h"
#include "status.h"
#include "memory/memory.h"

// Primary master disk
struct disk disk;

int disk_read_sector(int lba, int total, void* buf)
{
    // https://wiki.osdev.org/ATA_read/write_sectors

    outb(ATA_LBA_UPPER_BITS_PORT, (lba >> 24) | ATA_LBA_MODE);
    outb(ATA_NUMBER_OF_SECTORS_PORT, total);
    outb(ATA_LBA_LOW_BITS_PORT, (unsigned char)(lba & 0xff));
    outb(ATA_LBA_MIDDLE_BITS_PORT, (unsigned char)(lba >> 8));
    outb(ATA_LBA_HIGH_BITS_PORT, (unsigned char)(lba >> 16));
    outb(ATA_COMMAND_PORT, ATA_COMMAND_READ_WITH_RETRY);

    // read 2 bytes at a time
    unsigned short* ptr = (unsigned short*) buf;
    for (int b = 0; b < total; b++)
    {
        // Wait for the buffer to be ready
        char c = insb(ATA_COMMAND_PORT);
        while (!(c & ATA_SECTOR_BUFFER_REQUIRES_SERVICING))
        {
            c = insb(ATA_COMMAND_PORT);
        }

        // Copy from hard disk to memory
        for (int i = 0; i < 256; i++)
        {
            *ptr = insw(ATA_DATA_IN_OUT_PORT);
            ptr++;
        }
    }
    return 0;
}

void disk_search_and_init()
{
    memset(&disk, 0, sizeof(disk));
    disk.type = PEACHOS_DISK_TYPE_REAL;
    disk.sector_size = PEACHOS_SECTOR_SIZE;
    disk.id = 0;
    disk.filesystem = fs_resolve(&disk);
}

struct disk* disk_get(int index)
{
    if (index != 0)
        return NULL;
    
    return &disk;
}

int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf)
{
    if (idisk != &disk)
    {
        return -EIO;
    }

    return disk_read_sector(lba, total, buf);
}