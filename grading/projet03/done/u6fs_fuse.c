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

/**
 * @file u6fs_fuse.h
 * @brief Fills a stat struct with the attributes of a file
 *
 * @param path absolute path to the file
 * @param stbuf stat struct to fill
 * @return 0 on success, <0 on error
 */
int fs_getattr(const char *path, struct stat *stbuf)
{
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(stbuf);
    M_REQUIRE_NON_NULL(theFS);

    int inr = direntv6_dirlookup(theFS, ROOT_INUMBER, path);
    if(inr < 0){
        return inr;
    }
    else{
        struct inode i = {0};
        int err = inode_read(theFS, inr, &i);
        if(err < 0){
            return err;
        }else{
            memset(stbuf, 0, sizeof(struct stat));
            stbuf->st_ino = inr;
            mode_t type = i.i_mode & IFDIR ? S_IFDIR : S_IFREG;
            stbuf->st_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH | type;

            stbuf->st_nlink = i.i_nlink;

            stbuf->st_uid = i.i_uid;

            stbuf->st_gid = i.i_gid;

            size_t size = inode_getsize(&i);
            stbuf->st_size = size;

            stbuf->st_blocks = size % SECTOR_SIZE == 0 ? size/SECTOR_SIZE : size/SECTOR_SIZE + 1; 

            stbuf->st_blksize = SECTOR_SIZE;

            return ERR_NONE;
        }
    }
}

/**
 * @file u6fs_fuse.h
 * @brief Reads all the entries of a directory and output the result to a buffer using a given filler function
 *
 * @param path absolute path to the directory
 * @param buf buffer given to the filler function
 * @param filler function called for each entries, with the name of the entry and the buf parameter
 * @param offset ignored
 * @param fi fuse info -- ignored
 * @return 0 on success, <0 on error
 */
// Insert directory entries into the directory structure, which is also passed to it as buf
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset _unused, struct fuse_file_info *fi)
{
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(buf);
    M_REQUIRE_NON_NULL(filler);
    M_REQUIRE_NON_NULL(fi);
    M_REQUIRE_NON_NULL(theFS);

    int inr = direntv6_dirlookup(theFS, ROOT_INUMBER, path);
    if(inr < 0) {
        return inr;
    }

    struct directory_reader dr;
    memset(&dr, 0, sizeof(struct directory_reader));
    int res_opendir = direntv6_opendir(theFS, inr, &dr);
    if(res_opendir < 0) return res_opendir;

    char* name = calloc(DIRENT_MAXLEN + 1, 1);
    if(name == NULL) return ERR_NOMEM;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    int res_readdir;
    while((res_readdir = direntv6_readdir(&dr, name, &inr)) != 0){
        if(res_readdir < 0){
            free(name);
            return res_readdir;
        }

        int res_filler = filler(buf, name, NULL, 0);
        if(res_filler == 1){
            free(name);
            return ERR_NOMEM;
        }
    }
    free(name);
    return res_readdir;
}



/**
 * @file u6fs_fuse.h
 * @brief reads at most size bytes from a file into a buffer
 * @param path absolute path to the file
 * @param buf buffer where read bytes will be written
 * @param size size in bytes of the buffer
 * @param offset read offset in the file
 * @param fi fuse info -- ignored
 * @return number of bytes read on success, <0 on error
 */
int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(buf);
    M_REQUIRE_NON_NULL(theFS);
    M_REQUIRE_NON_NULL(fi);

    int inr = direntv6_dirlookup(theFS, ROOT_INUMBER, path);
    if(inr < 0){
        return inr;
    }

    struct filev6 f;
    memset(&f, 0, sizeof(struct filev6));
    int res = filev6_open(theFS, inr, &f);
    if(res < 0) return res;

    int err = filev6_lseek(&f, offset);
    if(err < 0) return err;

 
    size_t length = 0;
    int bytes_read = SECTOR_SIZE;
    char data[SECTOR_SIZE] = {0};
    while(bytes_read == SECTOR_SIZE && length < size){
        bytes_read = filev6_readblock(&f, data);
        if(bytes_read == SECTOR_SIZE){
            if(length + SECTOR_SIZE > size){
                strncpy(buf + length, data, size - length);
                length = size;
            }else{
                strncpy(buf + length, data, SECTOR_SIZE);
                length += SECTOR_SIZE;
            }
        }else if(bytes_read < 0){
            return bytes_read;
        }else{
            int remaining_length = ((size - length) > bytes_read) ? bytes_read : (size - length);
            strncpy(buf + length, data, remaining_length);
            length += remaining_length;
        }
    }
    return length;
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
