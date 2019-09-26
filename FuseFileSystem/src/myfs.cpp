//
//  myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 02.08.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

// The functions fuseGettattr(), fuseRead(), and fuseReadDir() are taken from
// an example by Mohammed Q. Hussain. Here are original copyrights & licence:

/**
 * Simple & Stupid Filesystem.
 *
 * Mohammed Q. Hussain - http://www.maastaar.net
 *
 * This is an example of using FUSE to build a simple filesystem. It is a part of a tutorial in MQH Blog with the title "Writing a Simple Filesystem Using FUSE in C": http://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/
 *
 * License: GNU GPL
 */

// For documentation of FUSE methods see https://libfuse.github.io/doxygen/structfuse__operations.html

#undef DEBUG

// TODO: Comment this to reduce debug messages
#define DEBUG
#define DEBUG_METHODS
#define DEBUG_RETURN_VALUES

#include <unistd.h>
#include <string.h>
#include <cerrno>
#include <helper.h>

#include "macros.h"
#include "myfs.h"
#include "myfs-info.h"
#include "serializer.h"

MyFS* MyFS::_instance = NULL;

MyFS* MyFS::Instance() {
    if(_instance == NULL) {
        _instance = new MyFS();
    }
    return _instance;
}

MyFS::MyFS() {
    this->logFile= stderr;
}

MyFS::~MyFS() {
    
}

int MyFS::fuseGetattr(const char *path, struct stat *statbuf) {
    LOGM();
    
    // TODO: Implement this!

    LOGF( "\tAttributes of %s requested\n", path );

    if ( strcmp( path, "/" ) == 0 )
    {
        statbuf->st_mode = S_IFDIR | 0555;
        statbuf->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
        statbuf->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
        statbuf->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
        return 0;
    }

    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (root.files[i].name[0] == 0) continue;

        if(strcmp(path+1, root.files[i].name)==0) {
            statbuf->st_mode = root.files[i].mode;
            statbuf->st_nlink = 1;
            statbuf->st_size = root.files[i].size;

            statbuf->st_uid = root.files[i].user_id;
            statbuf->st_gid = root.files[i].group_id;

            root.files[i].atime = (u_int64_t) time(NULL);

            statbuf->st_atime = root.files[i].atime;
            statbuf->st_mtime = root.files[i].mtime;
            statbuf->st_ctime = root.files[i].ctime;

            serializeHeaderToFile(blockDevice, this);

            return 0;
        }
    }

    return -ENOENT;
}

int MyFS::fuseReadlink(const char *path, char *link, size_t size) {
    LOGM();
    return 0;
}

int MyFS::fuseMknod(const char *path, mode_t mode, dev_t dev) {
    LOGM();

    LOGF("path : %s", path);

    const char* fileName = path + 1;

    if (strlen(fileName) > NAME_LENGTH) return -ENAMETOOLONG;
    if (fileNameExists(fileName)) return -EEXIST;

    int rootPos = 0;
    for (; root.files[rootPos].name[0] != 0; rootPos++);

    if (rootPos >= NUM_DIR_ENTRIES) return -EIO;
    File* file = &root.files[rootPos];

    int pos = 0;
    for (; fileName[pos] != 0; pos++)
        file->name[pos] = fileName[pos];
    file->name[pos] = 0;
    file->size = 0;

    file->user_id = getuid();
    file->group_id = getgid();
    file->mode = mode;

    // Aktuelle Zeit für atime und ctime
    // TODO: mtime von Datei übernehmen
    file->ctime = time(NULL);
    file->atime = time(NULL);
    file->mtime = time(NULL);

    serializeHeaderToFile(blockDevice, this);

    return 0;
}

int MyFS::fuseMkdir(const char *path, mode_t mode) {
    LOGM();
    return 0;
}

int MyFS::fuseUnlink(const char *path) {
    LOGM();

    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        File* file = &root.files[i];
        if (file->name[0] == 0) continue;
        LOGF("name: %s, path: %s, strcmp = %d", root.files[i].name, path, strcmp(path+1, root.files[i].name));

        if(strcmp(path+1, file->name)==0) {

            // Datei in der FAT und DMap zurücksetzen
            deleteFATEntries(file);

            for (int j = 0; j < NAME_LENGTH; j++)
                file->name[j] = 0;

            file->size = 0;
            file->first_block = FAT_EMPTY_ENTRY;
            file->mode = 0;
            file->group_id = 0;
            file->user_id = 0;
            file->ctime = 0;
            file->atime = 0;
            file->mtime = 0;

            serializeHeaderToFile(blockDevice, this);

            return 0;
        }
    }

    return -ENOENT;
}

int MyFS::fuseRmdir(const char *path) {
    LOGM();
    return 0;
}

int MyFS::fuseSymlink(const char *path, const char *link) {
    LOGM();
    return 0;
}

int MyFS::fuseRename(const char *path, const char *newpath) {
    LOGM();
    return 0;
}

int MyFS::fuseLink(const char *path, const char *newpath) {
    LOGM();
    return 0;
}

int MyFS::fuseChmod(const char *path, mode_t mode) {
    LOGM();
    return 0;
}

int MyFS::fuseChown(const char *path, uid_t uid, gid_t gid) {
    LOGM();
    return 0;
}

int MyFS::fuseTruncate(const char *path, off_t newSize) {
    LOGM();
    return 0;
}

int MyFS::fuseUtime(const char *path, struct utimbuf *ubuf) {
    LOGM();
    return 0;
}

int MyFS::fuseOpen(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();

    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        if (root.files[i].name[0] == 0) continue;
        LOGF("name: %s, path: %s, strcmp = %d", root.files[i].name, path, strcmp(path+1, root.files[i].name));

        if(strcmp(path+1, root.files[i].name)==0) {

            int index = 0;
            // Array für offene Dateien nach freiem Platz durchsuchen
            for (; index < NUM_OPEN_FILES; index++) {
                if (!openFiles.entries[index].filled) break;
            }
            // Wenn schon 64 Dateien offen sind, Fehler schmeißen
            if (index >= NUM_OPEN_FILES) return -EMFILE;

            openFiles.entries[index].filled = true;
            openFiles.entries[index].info = &(root.files[i]);
            openFiles.entries[index].blockInFile = 1;
            openFiles.entries[index].currentBlock = -1;

            LOGF("fh set to %d ...", index);
            fileInfo->fh = index;

            return 0;
        }
    }
    
    return -ENOENT;
}

int MyFS::fuseRead(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();

    LOGF( "--> Trying to read %s, %lu, %lu\n", path, offset, size );

    LOGF("fh at read: %ld", fileInfo->fh);

    if (fileInfo->fh < 0 || fileInfo->fh >= 64) return -EBADF;
    if (!openFiles.entries[fileInfo->fh].filled) return -EBADF;

    int index = fileInfo->fh;

    u_int32_t fileSize = openFiles.entries[index].info->size;
    int fileBlockCount = sizeInBlocks(fileSize);

    int startReadAtBlock = sizeInBlocks(offset);
    bool offsetNegative = offset < 0;
    bool offsetAfterFileBlocks = fileBlockCount < startReadAtBlock;
    bool offsetAfterFileEnd = fileBlockCount == startReadAtBlock && (fileSize % BLOCK_SIZE) < (offset % BLOCK_SIZE);

    if (offsetNegative || offsetAfterFileBlocks || offsetAfterFileEnd) return -ENXIO;

    int startReadBlock = openFiles.entries[index].info->first_block;
    for (int i = 0; i < startReadAtBlock; ++i) {
        startReadBlock = fat.entries[startReadBlock];
    }

    if (openFiles.entries[index].currentBlock != startReadBlock) {
        LOG("Loading first Block");
        openFiles.entries[index].currentBlock = startReadBlock;
        readBlockFromFile(startReadBlock, openFiles.entries[index].data);
    }
    else {
        LOG("Block is already loaded");
    }

    // wieviele Bytes sollen gelesen werden
    u_int32_t bytesToRead = size;

    // wieviele Bytes können gelesen werden
    u_int32_t bytesPossibleToRead = fileSize - offset;

    // das momentane Offset in der Datei
    u_int32_t currOffset = offset;

    u_int32_t bytesReadTotal = 0;
    while (bytesToRead > 0 && bytesPossibleToRead > 0) {

        int offsetInBlock = currOffset % BLOCK_SIZE;
        u_int32_t remainingBytesInBlock = BLOCK_SIZE - offsetInBlock;

        // legt die Anzahl der übrigen lesbaren Bytes in diesem Block fest
        remainingBytesInBlock =
                bytesPossibleToRead < remainingBytesInBlock ? bytesPossibleToRead : remainingBytesInBlock;

        if (offsetInBlock > remainingBytesInBlock) return -ENXIO;

        // wieviele Bytes können wirklich gelesen werden
        u_int32_t bytesRead = bytesToRead < remainingBytesInBlock ? bytesToRead : remainingBytesInBlock;

        memcpy(buf + bytesReadTotal, openFiles.entries[index].data + offsetInBlock, bytesRead);

        // alle Werte aktualisieren
        bytesToRead -= bytesRead;
        bytesPossibleToRead -= bytesRead;
        bytesReadTotal += bytesRead;

        currOffset += bytesRead;

        LOGF("bytes read %d, bytesReadTotal %d, bytesPossibleToRead %d, block %d",
             bytesRead, bytesReadTotal, bytesPossibleToRead, openFiles.entries[index].currentBlock);

        // lade nächsten Block, wenn noch Bytes zu lesen sind und der momentane Block fertig gelesen wurde
        if (bytesToRead > 0 && bytesRead == remainingBytesInBlock) {
            if (fat.entries[openFiles.entries[index].currentBlock] == FAT_EMPTY_ENTRY)
                break;

            openFiles.entries[index].currentBlock = fat.entries[openFiles.entries[index].currentBlock];
            LOG("Loading next block");
            readBlockFromFile(openFiles.entries[index].currentBlock, openFiles.entries[index].data);
        }
    }
    LOG("Done reading");

    return bytesReadTotal;
}

int MyFS::fuseWrite(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();

    LOGF( "--> Trying to write %s, %lu, %lu\n", path, offset, size);

    // TODO: 2. Aufgabe
    if (fileInfo->fh < 0 || fileInfo->fh >= 64) return -EBADF;
    if (!openFiles.entries[fileInfo->fh].filled) return -EBADF;

    int index = fileInfo->fh;

    int startWriteAtBlock = sizeInBlocks(offset);
    /*bool offsetNegative = offset < 0;
    bool offsetAfterFileBlocks = partBlockCount < startWriteAtBlock;
    bool offsetAfterFileEnd = partBlockCount == startWriteAtBlock && (partSize % BLOCK_SIZE) < (offset % BLOCK_SIZE);

    if (offsetNegative || offsetAfterFileBlocks || offsetAfterFileEnd) return -ENXIO;
*/

    if (openFiles.entries[index].info->first_block == FAT_EMPTY_ENTRY) {
        LOG("Set up inital block");
        int freeBlock = findFreeBlock();
        if (freeBlock < 0) return -ENOSPC;
        setBlockUsed(freeBlock);
        openFiles.entries[index].info->first_block = freeBlock;
        openFiles.entries[index].currentBlock = freeBlock;
    }
    else {
        LOG("Locating block");
        int freeBlock = findFreeBlock();
        if (offset == 0) {
            if (freeBlock < 0) return -ENOSPC;

            deleteFATEntries(openFiles.entries[index].info);

            openFiles.entries[index].info->first_block = freeBlock;
            openFiles.entries[index].currentBlock = freeBlock;
            setBlockUsed(freeBlock);
        }
        else {
            int startWriteBlock = openFiles.entries[index].info->first_block;
            for (int i = 0; i < startWriteAtBlock; ++i) {
                if (fat.entries[startWriteBlock] == FAT_EMPTY_ENTRY) break;
                startWriteBlock = fat.entries[startWriteBlock];
            }
            openFiles.entries[index].currentBlock = startWriteBlock;
            if (offset % BLOCK_SIZE == 0) {
                LOG("Start in new block");
                if (freeBlock < 0) return -ENOSPC;
                fat.entries[startWriteBlock] = freeBlock;
                openFiles.entries[index].currentBlock = freeBlock;
                setBlockUsed(freeBlock);
                for (int i = 0; i < BLOCK_SIZE; i++)
                    openFiles.entries[index].data[i] = 0;
            }
            else {
                LOG("Start in existing block");
                readBlockFromFile(startWriteBlock, openFiles.entries[index].data);
            }
        }
    }

    // wieviele Bytes sollen geschrieben werden
    u_int32_t bytesToWrite = size;

    // das momentane Offset in der Datei
    u_int32_t currOffset = offset;

    u_int32_t bytesWrittenTotal = 0;
    while (bytesToWrite > 0) {

        LOGF("first block %d, current block %d", openFiles.entries[index].info->first_block,
             openFiles.entries[index].currentBlock);

        int offsetInBlock = currOffset % BLOCK_SIZE;
        u_int32_t remainingBytesInBlock = BLOCK_SIZE - offsetInBlock;

        // wieviele Bytes können wirklich geschrieben werden
        u_int32_t bytesWritten = bytesToWrite < remainingBytesInBlock ? bytesToWrite : remainingBytesInBlock;

        memcpy(openFiles.entries[index].data + offsetInBlock, buf + bytesWrittenTotal, bytesWritten);
        writeBlockToFile(openFiles.entries[index].currentBlock, openFiles.entries[index].data);

        // alle Werte aktualisieren
        bytesToWrite -= bytesWritten;
        bytesWrittenTotal += bytesWritten;

        currOffset += bytesWritten;

        LOGF("bytes written %d, bytesWrittenTotal %d, block %d",
             bytesWritten, bytesWrittenTotal, openFiles.entries[index].currentBlock);

        // lade nächsten Block, wenn noch Bytes zu schreiben sind und der momentane Block fertig geschrieben wurde
        if (bytesToWrite > 0 && bytesWritten == remainingBytesInBlock) {
            u_int32_t nextBlock = fat.entries[openFiles.entries[index].currentBlock];
            if (nextBlock == FAT_EMPTY_ENTRY) {
                int freeBlock = findFreeBlock();
                if (freeBlock < 0) return -ENOSPC;
                setBlockUsed(freeBlock);
                int currentBlock = openFiles.entries[index].currentBlock;
                fat.entries[currentBlock] = freeBlock;

                openFiles.entries[index].currentBlock = freeBlock;
                LOG("Next block is allocated");
            }
            else {
                openFiles.entries[index].currentBlock = nextBlock;
                readBlockFromFile(nextBlock, openFiles.entries[index].data);
                LOG("Next block exists");
            }
            LOG("Next block is set up");
        }
    }
    LOG("Done writing");
    openFiles.entries[index].info->size = offset + bytesWrittenTotal;
    serializeHeaderToFile(blockDevice, this);
    return bytesWrittenTotal;
}

int MyFS::fuseStatfs(const char *path, struct statvfs *statInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseFlush(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseRelease(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();

    if (fileInfo->fh < 0 || fileInfo->fh >= 64) return -EBADF;
    if (!openFiles.entries[fileInfo->fh].filled) return -EBADF;

    LOGF("Called release for fh %ld", fileInfo->fh);
    if (fileInfo->fh >= 0) {
        openFiles.entries[fileInfo->fh].filled = false;
        openFiles.entries[fileInfo->fh].info = nullptr;
        return 0;
    }

    return -ENOENT;
}

int MyFS::fuseFsync(const char *path, int datasync, struct fuse_file_info *fi) {
    LOGM();
    return 0;
}

int MyFS::fuseListxattr(const char *path, char *list, size_t size) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseRemovexattr(const char *path, const char *name) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseOpendir(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseReaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();

    LOGF( "--> Getting The List of Files of %s\n", path );

    filler( buf, ".", NULL, 0 );
    filler( buf, "..", NULL, 0 );

    if ( strcmp( path, "/" ) == 0 )
    {
        for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
            if (root.files[i].name[0] == 0) continue;

            filler(buf, root.files[i].name, NULL, 0);
        }
        return 0;
    }

    return -ENOTDIR;
}

int MyFS::fuseReleasedir(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseFsyncdir(const char *path, int datasync, struct fuse_file_info *fileInfo) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseTruncate(const char *path, off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseCreate(const char *path, mode_t mode, struct fuse_file_info *fileInfo) {
    LOGM();
    
    return -ENOSYS;
}

void MyFS::fuseDestroy() {
    LOGM();
}

void* MyFS::fuseInit(struct fuse_conn_info *conn) {
    // Open logfile
    this->logFile= fopen(((MyFsInfo *) fuse_get_context()->private_data)->logFile, "w+");
    if(this->logFile == NULL) {
        fprintf(stderr, "ERROR: Cannot open logfile %s\n", ((MyFsInfo *) fuse_get_context()->private_data)->logFile);
    } else {
        //    this->logFile= ((MyFsInfo *) fuse_get_context()->private_data)->logFile;
        
        // turn of logfile buffering
        setvbuf(this->logFile, NULL, _IOLBF, 0);
        
        LOG("Starting logging...\n");
        LOGM();

        LOG("Initializing structures ...");

        // you can get the containfer file name here:
        char *containerFileName = ((MyFsInfo *) fuse_get_context()->private_data)->contFile;
        LOGF("Container file name: %s", containerFileName);
        
        // TODO: Implement your initialization methods here!
        LOG("Creating blockdevice ...\n");
        blockDevice = new BlockDevice();
        blockDevice->open(containerFileName);

        LOG("Deserializing header blocks ...\n");
        deserializeHeaderFromFile(blockDevice, this);

        LOG("Initialization is done\n");
    }
    
    RETURN(0);
}

#ifdef __APPLE__
int MyFS::fuseSetxattr(const char *path, const char *name, const char *value, size_t size, int flags, uint32_t x) {
#else
int MyFS::fuseSetxattr(const char *path, const char *name, const char *value, size_t size, int flags) {
#endif
    LOGM();
    RETURN(0);
}
    
#ifdef __APPLE__
int MyFS::fuseGetxattr(const char *path, const char *name, char *value, size_t size, uint x) {
#else
int MyFS::fuseGetxattr(const char *path, const char *name, char *value, size_t size) {
#endif
    LOGM();
    RETURN(0);
}
        
// TODO: Add your own additional methods here!

int MyFS::getDriveSpace() {
    return superblock.size;
}

int MyFS::readBlockFromFile(u_int32_t block, char* buffer) {
    u_int32_t actualBlock = block + superblock.data_start_block;
    if (block < DATA_BLOCK_COUNT) {
        blockDevice->read(actualBlock, buffer);
        return 0;
    }

    return -ENOMEM;
}

int MyFS::writeBlockToFile(u_int32_t block, char* buffer) {
    u_int32_t actualBlock = block + superblock.data_start_block;
    if (block < DATA_BLOCK_COUNT) {
        blockDevice->write(actualBlock, buffer);
        return 0;
    }

    return -ENOMEM;
}

void MyFS::initializeStructures() {
    for (u_int32_t i = 0; i < DATA_BLOCK_COUNT; i++) {
        dmap.is_used[i] = false;
        fat.entries[i] = FAT_EMPTY_ENTRY;
    }

    for (int i = 0; i < NUM_DIR_ENTRIES; i++) {
        for (int j = 0; j < NAME_LENGTH; j++)
            root.files[i].name[j] = 0;

        root.files[i].first_block = FAT_EMPTY_ENTRY;
        root.files[i].size = 0;
        root.files[i].mode = 0;
        root.files[i].mtime = 0;
        root.files[i].ctime = 0;
        root.files[i].atime = 0;
        root.files[i].user_id = 0;
        root.files[i].group_id = 0;
    }
}

bool MyFS::fileNameExists(const char* fileName) {
    for (int i=0; i < NUM_DIR_ENTRIES; i++){
        if (root.files[i].name[0] == 0) continue;

        if (strcmp(root.files[i].name, fileName) == 0)
            return true;
    }
    return false;
}

int MyFS::findFreeBlock() {
    int block = 0;
    while (dmap.is_used[block]) {
        block++;
    }
    return block >= DATA_BLOCK_COUNT ? -ENOSPC: block;
}

u_int32_t MyFS::countFreeBlocks() {
    u_int32_t count = 0;
    for (u_int32_t i = 0; i < DATA_BLOCK_COUNT; i++)
        if (!dmap.is_used[i]) count++;

    return count;
}

void MyFS::setBlockFree(u_int32_t block) {
    dmap.is_used[block] = false;
}

void MyFS::setBlockUsed(u_int32_t block) {
    dmap.is_used[block] = true;
}

void MyFS::deleteFATEntries(File *file) {
    u_int32_t curr = file->first_block;
    if (curr == FAT_EMPTY_ENTRY) return;

    while (fat.entries[curr] != FAT_EMPTY_ENTRY) {
        setBlockFree(curr);

        u_int32_t prev = curr;
        curr = fat.entries[curr];
        fat.entries[prev] = FAT_EMPTY_ENTRY;
    }
    setBlockFree(curr);
    file->first_block = FAT_EMPTY_ENTRY;
}