/**
 * @file u6fs_utils.c
 * @brief Utilities (mostly dump) for UV6 filesystem
 * @author Aurélien Soccard / EB
 * @date 2022
 */

#include <string.h> // memset
#include <inttypes.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include "mount.h"
#include "sector.h"
#include "error.h"
#include "u6fs_utils.h"
#include "filev6.h"
#include "inode.h"
#include "unixv6fs.h"

int utils_print_superblock(const struct unix_filesystem *u)
{
    M_REQUIRE_NON_NULL(u);
    pps_printf("**********FS SUPERBLOCK START**********\n");
    pps_printf("%-20s: %" PRIu16 "\n", "s_isize",       u->s.s_isize      );
    pps_printf("%-20s: %" PRIu16 "\n", "s_fsize",       u->s.s_fsize      );
    pps_printf("%-20s: %" PRIu16 "\n", "s_fbmsize",     u->s.s_fbmsize    );
    pps_printf("%-20s: %" PRIu16 "\n", "s_ibmsize",     u->s.s_ibmsize    );
    pps_printf("%-20s: %" PRIu16 "\n", "s_inode_start", u->s.s_inode_start);
    pps_printf("%-20s: %" PRIu16 "\n", "s_block_start", u->s.s_block_start);
    pps_printf("%-20s: %" PRIu16 "\n", "s_fbm_start",   u->s.s_fbm_start  );
    pps_printf("%-20s: %" PRIu16 "\n", "s_ibm_start",   u->s.s_ibm_start  );
    pps_printf("%-20s: %" PRIu8 "\n", "s_flock",        u->s.s_flock      );
    pps_printf("%-20s: %" PRIu8 "\n", "s_ilock",        u->s.s_ilock      );
    pps_printf("%-20s: %" PRIu8 "\n", "s_fmod",         u->s.s_fmod       );
    pps_printf("%-20s: %" PRIu8 "\n", "s_ronly",        u->s.s_ronly      );
    pps_printf("%-20s: [%" PRIu16 "] %" PRIu16 "\n", "s_time", u->s.s_time[0], u->s.s_time[1]);
    pps_printf("**********FS SUPERBLOCK END**********\n");
    return ERR_NONE;
}

static void utils_print_SHA_buffer(unsigned char *buffer, size_t len)
{
    unsigned char sha[SHA256_DIGEST_LENGTH];
    SHA256(buffer, len, sha);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        pps_printf("%02x", sha[i]);
    }
    pps_printf("\n");
}



/**
 * @brief print to stdout the content of the an inode
 * @param u - the mounted filesytem
 * @param inr - the inode number
 * @return 0 on success, <0 on error
 */
int utils_print_inode(const struct inode *in){
    pps_printf("**********FS INODE START**********\n");
    if(in == NULL){
        pps_printf("NULL ptr\n");
        pps_printf("**********FS INODE END************\n");
        return ERR_BAD_PARAMETER;
    }   
    else{
        pps_printf("i_mode: %hu\n", in->i_mode);
        pps_printf("i_nlink: %hhu\n", in->i_nlink);
        pps_printf("i_uid: %hhu\n", in->i_uid);
        pps_printf("i_gid: %hhu\n", in->i_gid);
        pps_printf("i_size0: %hhu\n", in->i_size0);
        pps_printf("i_size1: %hu\n", in->i_size1);
        pps_printf("size: %" PRId32 "\n", inode_getsize(in));
        pps_printf("**********FS INODE END************\n");
        return ERR_NONE;
    }
   
}

/**
 * @brief print to stdout the first sector of a file
 * @param u - the mounted filesytem
 * @param inr - the inode number of the file
 * @return 0 on success, <0 on error
 */
int utils_cat_first_sector(const struct unix_filesystem *u, uint16_t inr){
    struct filev6 f;
    memset(&f, 0, sizeof(struct filev6));
    int res = filev6_open(u, inr, &f);
    if(res == ERR_NONE){
        pps_printf("\nPrinting inode #%d:\n", inr);
        int err = utils_print_inode(&(f.i_node));
        if(err < 0) return err;
        if(f.i_node.i_mode & IFDIR){
            pps_printf("which is a directory.\n");
        }
        else{
            pps_printf("the first sector of data of which contains:\n");
            unsigned char tab[SECTOR_SIZE];
            int number = filev6_readblock(&f, tab);
            if(number < 0){
                return number;
            }
            for(size_t i = 0; i < number; i++){
                pps_printf("%c", tab[i]);
            }
            pps_printf("----\n");
        }
    }
    else{
        pps_printf("filev6_open failed for inode #%d.\n", inr);      
    }
    return res;
}

/**
 * @brief print to stdout the SHA256 digest of the first UTILS_HASHED_LENGTH bytes of the file
 * @param u - the mounted filesystem
 * @param inr - the inode number of the file
 * @return 0 on success, <0 on error
 */
int utils_print_shafile(const struct unix_filesystem *u, uint16_t inr){
    M_REQUIRE_NON_NULL(u);
    struct filev6 f;
    memset(&f, 0, sizeof(struct filev6));
    int err = filev6_open(u, inr, &f);
    if(err == ERR_NONE){
        if (f.i_node.i_mode & IFDIR){
            pps_printf("SHA inode %d: %s\n", inr, SHORT_DIR_NAME);
            return ERR_NONE;
        }
        else{
            unsigned char buffer[UTILS_HASHED_LENGTH];
            size_t length = 0;
            int res = SECTOR_SIZE;
            while(res == SECTOR_SIZE && length < UTILS_HASHED_LENGTH){
                res = filev6_readblock(&f, buffer + length);
                if(res == SECTOR_SIZE){
                    if(length + SECTOR_SIZE > UTILS_HASHED_LENGTH){
                        length = UTILS_HASHED_LENGTH;
                    }else{
                        length += SECTOR_SIZE;
                    }
                }else if(res < 0){
                    return res;
                }else{
                    length += res;
                }
            }
            pps_printf("SHA inode %d: ", inr);
            utils_print_SHA_buffer(buffer,length);
            return ERR_NONE;
        }
    }else{
        return err;
    }
}


/**
 * @brief print to stdout the SHA256 digest of all files, sorted by inode number
 * @param u - the mounted filesystem
 * @return 0 on success, <0 on error
 */
int utils_print_sha_allfiles(const struct unix_filesystem *u){
    M_REQUIRE_NON_NULL(u);
    pps_printf("Listing inodes SHA\n");
    for(size_t i = ROOT_INUMBER; i < (u->s).s_isize * INODES_PER_SECTOR;++i){
        int err = utils_print_shafile(u, i);
        if(err != ERR_UNALLOCATED_INODE && err != ERR_NONE){
            return err;
        }
    }
    return ERR_NONE;
}

/**
 * @brief print to stdout the inode and sector bitmaps
 * @param u - the mounted filesystem
 * @return 0 on success, <0 on error
 */
int utils_print_bitmaps(const struct unix_filesystem *u){
    M_REQUIRE_NON_NULL(u);
    bm_print("INODES",u->ibm);
    bm_print("SECTORS", u->fbm);
    return ERR_NONE;
}
