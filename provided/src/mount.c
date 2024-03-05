/**
 * @file mount.c
 * @brief mounts the filesystem
 *
 * @author Ludovic Mermod / Aurélien Soccard / Edouard Bugnion
 * @date 2022
 */

#include <string.h> // memset()
#include <inttypes.h>

#include "error.h"
#include "mount.h"
#include "sector.h"
#include "util.h" // for TO_BE_IMPLEMENTED() -- remove later

int mountv6(const char *filename, struct unix_filesystem *u)
{
    TO_BE_IMPLEMENTED();
    return ERR_INVALID_COMMAND;
}

int umountv6(struct unix_filesystem *u)
{
    TO_BE_IMPLEMENTED();
    return ERR_INVALID_COMMAND;
}
