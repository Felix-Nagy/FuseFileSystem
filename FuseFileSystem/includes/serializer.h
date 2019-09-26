

#ifndef MYFS_SERIALIZER_H
#define MYFS_SERIALIZER_H

#include "sys/types.h"
#include "myfs-structs.h"
#include "myfs.h"
#include "blockdevice.h"

// SERIALIZE METHODS

void writeXToFile(BlockDevice* blockDevice, char* write_buffer, u_int32_t bufferSize, int start, int end);
void serializeStructToFile(BlockDevice* blockDevice, void* src, u_int32_t bufferSize, u_int32_t startBlock, u_int32_t endBlock);
void serializeStruct(void* src, char *buffer, u_int32_t bufferSize);

// you probably only need this one
void serializeHeaderToFile(BlockDevice* blockDevice, MyFS* myfs);


// DESERIALIZE METHODS

void readXFromFile(BlockDevice* blockDevice, char* read_buffer, u_int32_t bufferSize, int start, int end);
void deserializeStructFromFile(BlockDevice* blockDevice, void* src, u_int32_t bufferSize, u_int32_t startBlock, u_int32_t endBlock);
void deserializeStruct(void* src, char *buffer, u_int32_t bufferSize);

// you probably only need this one
void deserializeHeaderFromFile(BlockDevice* blockDevice, MyFS* myfs);

#endif //MYFS_SERIALIZER_H
