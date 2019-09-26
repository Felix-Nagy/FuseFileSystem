//
//  myfs-structs.h
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#ifndef myfs_structs_h
#define myfs_structs_h

#include "stdint.h"

#define NAME_LENGTH 255
#define BLOCK_SIZE 512
#define NUM_DIR_ENTRIES 64
#define NUM_OPEN_FILES 64

// TODO: Add structures of your file system here

#define DATA_SPACE_IN_MB 32
#define DATA_SPACE_IN_BYTES DATA_SPACE_IN_MB * (1 << 20)
#define DATA_BLOCK_COUNT DATA_SPACE_IN_BYTES / BLOCK_SIZE
#define FAT_EMPTY_ENTRY UINT32_MAX

struct DMap {
    bool is_used[DATA_BLOCK_COUNT];
};

// TODO: Sollte mit FAT_EMPTY_ENTRY initialisiert werden
struct FAT {
    u_int32_t entries[DATA_BLOCK_COUNT];
};

struct File {
    char name[NAME_LENGTH+1];
    int size = 0;
    int user_id = 0;
    int group_id = 0;
    mode_t mode = 0;
    u_int64_t atime = 0;
    u_int64_t mtime = 0;
    u_int64_t ctime = 0;
    u_int32_t first_block = FAT_EMPTY_ENTRY;
};

struct Root {
    File files[NUM_DIR_ENTRIES];
};


#define SUPER_BLOCK_COUNT (sizeof(Superblock) / BLOCK_SIZE) + (sizeof(Superblock) % BLOCK_SIZE > 0 ? 1 : 0)
#define DMAP_BLOCK_COUNT (sizeof(DMap) / BLOCK_SIZE) + (sizeof(DMap) % BLOCK_SIZE > 0 ? 1 : 0)
#define FAT_BLOCK_COUNT (sizeof(FAT) / BLOCK_SIZE) + (sizeof(FAT) % BLOCK_SIZE > 0 ? 1 : 0)
#define ROOT_BLOCK_COUNT (sizeof(Root) / BLOCK_SIZE) + (sizeof(Root) % BLOCK_SIZE > 0 ? 1 : 0)


#define HEADER_BLOCK_COUNT \
    SUPER_BLOCK_COUNT\
    + DMAP_BLOCK_COUNT\
    + FAT_BLOCK_COUNT\
    + ROOT_BLOCK_COUNT

#define DRIVE_BLOCK_COUNT HEADER_BLOCK_COUNT + DATA_BLOCK_COUNT
#define DRIVE_SPACE (HEADER_BLOCK_COUNT + DATA_BLOCK_COUNT) * BLOCK_SIZE

struct Superblock {
    u_int32_t size = DRIVE_SPACE;
    u_int32_t dmap_start_block = 1;
    u_int32_t fat_start_block = dmap_start_block + DMAP_BLOCK_COUNT;
    u_int32_t root_start_block = fat_start_block + FAT_BLOCK_COUNT;
    u_int32_t data_start_block = root_start_block + ROOT_BLOCK_COUNT;
};

struct OpenFile {
    bool filled = false; // flag to check if this struct is filled
    File* info = nullptr;
    u_int32_t currentBlock = FAT_EMPTY_ENTRY; // block that is currently in data
    u_int32_t blockInFile = FAT_EMPTY_ENTRY;  //  place of the block in the whole file
    char data[BLOCK_SIZE];  // currently read data
};

struct OpenFiles {
    OpenFile entries[NUM_OPEN_FILES];
};

#endif /* myfs_structs_h */
