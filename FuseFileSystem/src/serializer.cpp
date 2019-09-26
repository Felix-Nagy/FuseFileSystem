
#include "serializer.h"
#include "cstring"
#include "myfs.h"

void serializeHeaderToFile(BlockDevice* blockDevice, MyFS* myfs)  {
    serializeStructToFile(blockDevice, &(myfs->superblock), sizeof(Superblock), 0, myfs->superblock.dmap_start_block);
    serializeStructToFile(blockDevice, &(myfs->dmap), sizeof(DMap), myfs->superblock.dmap_start_block, myfs->superblock.fat_start_block);
    serializeStructToFile(blockDevice, &(myfs->fat), sizeof(FAT), myfs->superblock.fat_start_block, myfs->superblock.root_start_block);
    serializeStructToFile(blockDevice, &(myfs->root), sizeof(Root), myfs->superblock.root_start_block, myfs->superblock.data_start_block);
}

void serializeStructToFile(BlockDevice* blockDevice, void* src, u_int32_t bufferSize, u_int32_t startBlock, u_int32_t endBlock) {
    char *input_buffer = new char[bufferSize];
    for (int i = 0; i < bufferSize; i++) input_buffer[i] = 0;
    serializeStruct(src, input_buffer, bufferSize);
    writeXToFile(blockDevice, input_buffer, bufferSize, startBlock, endBlock);
    delete [] input_buffer;
}

void serializeStruct(void* src, char *buffer, u_int32_t bufferSize) {
    std::memcpy(buffer, src, bufferSize);
}

void writeXToFile(BlockDevice* blockDevice, char* write_buffer, u_int32_t bufferSize, int start, int end) {
    int offset = 0;
    u_int32_t bytesToWrite = bufferSize;
    for (int block = start; block < end; block++) {
        char *buffer = new char[BLOCK_SIZE];
        for (int i = 0; i < BLOCK_SIZE; i++) buffer[i] = 0;

        u_int32_t currentBytesToWrite = BLOCK_SIZE;
        if (bytesToWrite <= BLOCK_SIZE)
            currentBytesToWrite = bytesToWrite;
        else
            bytesToWrite -= BLOCK_SIZE;

        std::memcpy(buffer, write_buffer + (offset * BLOCK_SIZE), currentBytesToWrite);
        blockDevice->write(block, buffer);

        delete[] buffer;
        offset++;
    }
}

void deserializeHeaderFromFile(BlockDevice* blockDevice, MyFS* myfs) {
    deserializeStructFromFile(blockDevice, &(myfs->superblock), sizeof(Superblock), 0, 1);
    deserializeStructFromFile(blockDevice, &(myfs->dmap), sizeof(DMap), myfs->superblock.dmap_start_block, myfs->superblock.fat_start_block);
    deserializeStructFromFile(blockDevice, &(myfs->fat), sizeof(FAT), myfs->superblock.fat_start_block, myfs->superblock.root_start_block);
    deserializeStructFromFile(blockDevice, &(myfs->root), sizeof(Root), myfs->superblock.root_start_block, myfs->superblock.data_start_block);
}

void deserializeStructFromFile(BlockDevice* blockDevice, void* dest, u_int32_t bufferSize, u_int32_t startBlock, u_int32_t endBlock) {
    char *input_buffer = new char[bufferSize];
    for (int i = 0; i < bufferSize; i++) input_buffer[i] = 0;
    readXFromFile(blockDevice, input_buffer, bufferSize, startBlock, endBlock);
    deserializeStruct(dest, input_buffer, bufferSize);
    delete [] input_buffer;
}

void deserializeStruct(void* dest, char *buffer, u_int32_t bufferSize) {
    std::memcpy(dest, buffer, bufferSize);
}

void readXFromFile(BlockDevice* blockDevice, char* read_buffer, u_int32_t bufferSize, int start, int end) {
    int offset = 0;
    u_int32_t bytesToRead = bufferSize;
    for (int block = start; block < end; block++) {
        char *buffer = new char[BLOCK_SIZE];
        for (int i = 0; i < BLOCK_SIZE; i++) buffer[i] = 0;

        u_int32_t currentBytesToRead = BLOCK_SIZE;
        if (bytesToRead <= BLOCK_SIZE)
            currentBytesToRead = bytesToRead;
        else
            bytesToRead -= BLOCK_SIZE;

        blockDevice->read(block, buffer);
        std::memcpy(read_buffer + (offset * BLOCK_SIZE), buffer, currentBytesToRead);

        delete[] buffer;
        offset++;
    }
}