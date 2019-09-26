

#ifndef MYFS_HELPER_H
#define MYFS_HELPER_H

#include "unistd.h"
#include <sys/param.h>
#include "myfs-structs.h"

u_int32_t sizeInBlocks(u_int32_t sizeInBytes);

#endif //MYFS_HELPER_H
