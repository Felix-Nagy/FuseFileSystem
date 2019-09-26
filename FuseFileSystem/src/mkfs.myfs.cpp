//
//  mk.myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#include <iostream>
#include "myfs.h"
#include "blockdevice.h"
#include "macros.h"
#include "serializer.h"
#include "helper.h"
#include <libgen.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int insertFile(BlockDevice* blockDevice, MyFS* myfs, char* path) {
    std::cout << "Start insertFile" << std::endl;
    char *fileName = basename(path);

    if (strlen(fileName) > NAME_LENGTH) return -ENAMETOOLONG;
    if (myfs->fileNameExists(fileName)) return -EEXIST;

    std::cout << "Looking for a free place in root" << std::endl;
    // Suche freien Platz in Root
    int rootPos = 0;
    for (; myfs->root.files[rootPos].name[0] != 0; rootPos++);

    if (rootPos >= NUM_DIR_ENTRIES) return -EIO;
    File* file = &myfs->root.files[rootPos];

    // Datei-Eigenschaften auslesen
    struct stat *fileStatBuffer;
    fileStatBuffer= new struct stat;
    int fileStats = stat(path, fileStatBuffer);

    if (fileStats < 0) return -ENOENT;

    std::cout << "Count free blocks" << std::endl;
    // Freie Bloecke zaehlen
    u_int32_t freeBlockCount = myfs->countFreeBlocks();

    std::cout << "Counted free blocks: " << freeBlockCount << std::endl;
    // Groesse holen
    u_int32_t fileSize = fileStatBuffer->st_size;

    u_int32_t neededBlocks = sizeInBlocks(fileSize);
    if (neededBlocks > freeBlockCount) return -EFBIG;

    // Buffer fuer Datei erstellen
    u_int32_t actualFileSizeInBytes = neededBlocks * BLOCK_SIZE;
    char* buffer = new char[actualFileSizeInBytes];
    FILE *ptr;

    // Datei oeffnen
    ptr = fopen(path, "rb");  // r for read, b for binary
    // Daten aus Datei in Buffer lesen
    fread(buffer, actualFileSizeInBytes, 1, ptr);

    std::cout << "Copying data ..." << std::endl;
    u_int32_t currBlock = -1;
    u_int32_t prevBlock = -1;
    for (int i = 0; i < neededBlocks; i++) { //Buffer abarbeiten

        // Suche freien Block in DMAP
        std::cout << "Looking for free block ... " << std::endl;
        currBlock = myfs->findFreeBlock();
        std::cout << "Found free data block " << currBlock << std::endl;

        //DMAP schreiben
        myfs->setBlockUsed(currBlock);
        if (i == 0) file->first_block = currBlock;

        //FAT schreiben
        if (i > 0) myfs->fat.entries[prevBlock] = currBlock;

        int start = myfs->superblock.data_start_block + currBlock;
        std::cout << "Write to block " << start << std::endl;
        writeXToFile(blockDevice, buffer, BLOCK_SIZE, start, start+1);

        prevBlock = currBlock;
        buffer += BLOCK_SIZE;
    }
    if (fileSize == 0) {
        // suche freien Block in DMAP
        std::cout << "Looking for free block ... " << std::endl;
        currBlock = myfs->findFreeBlock();
        std::cout << "Found free data block " << currBlock << std::endl;

        //DMAP schreiben
        myfs->setBlockUsed(currBlock);
        file->first_block = currBlock;
    }
    myfs->fat.entries[currBlock] = FAT_EMPTY_ENTRY;

    // Root-Eigenschaften schreiben
    // Namen kopieren
    int pos = 0;
    for (; fileName[pos] != 0; pos++)
        file->name[pos] = fileName[pos];
    file->name[pos] = 0;

    file->size = fileStatBuffer->st_size;
    file->user_id = getuid();
    file->group_id = getgid();
    file->mode = S_IFREG | 0444;

    // Aktuelle Zeit für atime und ctime, mtime uebernehmen
    file->ctime = time(NULL);
    file->atime = time(NULL);

#ifdef __APPLE__
    file->mtime = fileStatBuffer->st_mtimespec.tv_sec;
#else
    file->mtime = fileStatBuffer->st_mtim.tv_sec;
#endif

    std::cout << fileName << " has been copied\n";
    return 0;
}

void printErrorMsg(int err, int argc, int fileIndex) {
    char msgDone[] = "Done.";
    char msgInval[] = "Warning: Empty container created. No files have been provided.";
    char msg2Big[] = "Too many arguments. You can only copy a maximum of 64 files.";
    char msgNameTooLong[] = "File name is too long.";
    char msgExists[] = "File already exists in the filesystem.";
    char msgNoEntry[] = "File to be copied cannot be found.";
    char msgIO[] = "There are already 64 files in the filesystem.";
    char msgFBig[] = "File cannot be copied, it is too big.";

    switch (err) {
        case 0: std::cout << msgDone << std::endl; break;
        case -EINVAL: std::cout <<  msgInval << " argc=" << argc << std::endl; break;
        case -E2BIG: std::cout << msg2Big << " argc=" << argc << std::endl; break;
        case -ENAMETOOLONG: std::cout << msgNameTooLong << " fileIndex=" << fileIndex << std::endl; break;
        case -EEXIST: std::cout << msgExists << " fileIndex=" << fileIndex << std::endl; break;
        case -ENOENT: std::cout << msgNoEntry << " fileIndex=" << fileIndex << std::endl; break;
        case -EIO: std::cout << msgIO << " fileIndex=" << fileIndex << std::endl; break;
        case -EFBIG: std::cout << msgFBig << " fileIndex=" << fileIndex << std::endl; break;
        default: std::cout << "Something went wrong. err=" << err << " argc=" << argc << " fileIndex=" << fileIndex << std::endl;
    }
}

int main(int argc, char *argv[]) {
    // TODO: Implement file system generation & copying of files here
    auto* myfs = MyFS::Instance();
    myfs->initializeStructures();

    auto* blockDevice = new BlockDevice();

    int err = 0;
    int argErr = 0;
    int i = 0;

    if (argc >= 2) blockDevice->create(argv[1]);
    else {
        std::cout << "No parameter for container file" << std::endl;
        return -EINVAL;
    };

    if(argc >= 3) {
        if(argc < NUM_DIR_ENTRIES + 3)
            for (i = 2; i < argc; i++) {
                err = insertFile(blockDevice, myfs, argv[i]);
                std::cout << "File insertion returned. Status: ";
                printErrorMsg(err, argc, i-1);
            }
        else
            argErr = -E2BIG;
    }
    else
        argErr = -EINVAL;

    printErrorMsg(argErr, argc, i-1);

    serializeHeaderToFile(blockDevice, myfs);

    blockDevice->close();

    delete blockDevice;
    delete myfs;

    return err;
}