/**
 * @file u6fs_fuse.c
 * @brief interface to FUSE (Filesystem in Userspace)
 *
 * @date 2022
 * @author Édouard Bugnion, Ludovic Mermod
 *  Inspired from hello.c from:
 *    FUSE: Filesystem in Userspace
 *    Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
 *
 *  This program can be distributed under the terms of the GNU GPL.
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <fcntl.h>

#include <stdlib.h> // for exit()
#include "mount.h"
#include "error.h"
#include "inode.h"
#include "direntv6.h"
#include "u6fs_utils.h"
#include "u6fs_fuse.h"
#include "util.h"

static struct unix_filesystem* theFS = NULL; // usefull for tests

int fs_getattr(const char *path, struct stat *stbuf)
{
    TO_BE_IMPLEMENTED();
    return ERR_INVALID_COMMAND;
}

// Insert directory entries into the directory structure, which is also passed to it as buf
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset _unused, struct fuse_file_info *fi)
{
    TO_BE_IMPLEMENTED();
    return ERR_INVALID_COMMAND;
}

int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    TO_BE_IMPLEMENTED();
    return ERR_INVALID_COMMAND;
}

static struct fuse_operations available_ops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .read    = fs_read,
};

int u6fs_fuse_main(struct unix_filesystem *u, const char *mountpoint)
{
    M_REQUIRE_NON_NULL(mountpoint);

    theFS = u;  // /!\ GLOBAL ASSIGNMENT
    const char *argv[] = {
        "u6fs",
        "-s",               // * `-s` : single threaded operation
        "-f",              // foreground operation (no fork).  alternative "-d" for more debug messages
        "-odirect_io",      //  no caching in the kernel.
#ifdef DEBUG
        "-d",
#endif
        //  "-ononempty",    // unused
        mountpoint
    };
    // very ugly trick when a cast is required to avoid a warning
    void *argv_alias = argv;

    utils_print_superblock(theFS);
    int ret = fuse_main(sizeof(argv) / sizeof(char *), argv_alias, &available_ops, NULL);
    theFS = NULL; // /!\ GLOBAL ASSIGNMENT
    return ret;
}

#ifdef CS212_TEST
void fuse_set_fs(struct unix_filesystem *u)
{
    theFS = u;
}
#endif
