

#include <sys/param.h>
#include "helper.h"

u_int32_t sizeInBlocks(u_int32_t sizeInBytes) {
    return (sizeInBytes / BLOCK_SIZE) + (sizeInBytes % BLOCK_SIZE > 0 ? 1 : 0);
}
