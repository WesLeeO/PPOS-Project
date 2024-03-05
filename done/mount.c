/**
 * @file mount.c
 * @brief mounts the filesystem
 *
 * @author Ludovic Mermod / Aurélien Soccard / Edouard Bugnion
 * @date 2022, spring 2023
 */

#include <string.h> // memset()
#include <inttypes.h>
#include "unixv6fs.h"
#include "error.h"
#include "mount.h"
#include "sector.h"
#include "bmblock.h"
#include "inode.h"

/**
 * @brief  mount a unix v6 filesystem
 * @param filename name of the unixv6 filesystem on the underlying disk (IN)
 * @param u the filesystem (OUT)
 * @return 0 on success; <0 on error
 */
int mountv6(const char *filename, struct unix_filesystem *u)
{
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(u);
    memset(u, 0, sizeof(*u));
    u->f = fopen(filename, "rb+");
    if (u->f == NULL) return ERR_IO;
    
    char data[SECTOR_SIZE];
    int err = sector_read(u->f, BOOTBLOCK_SECTOR, data);
    if (err == ERR_NONE) {
        if (data[BOOTBLOCK_MAGIC_NUM_OFFSET] != BOOTBLOCK_MAGIC_NUM){
            fclose(u->f);
            return ERR_BAD_BOOT_SECTOR;
        }
        else{
            struct superblock superblock_copy = {0};
            struct superblock *data1 = &superblock_copy;
            int err1 = sector_read(u->f, SUPERBLOCK_SECTOR, data1);
            if (err1 == ERR_NONE){
                u->s = *data1;

                u->ibm = bm_alloc(ROOT_INUMBER, u->s.s_isize * INODES_PER_SECTOR);
                if (u->ibm == NULL) {
                    fclose(u->f);
                    return ERR_NOMEM;
                }
                    
                u->fbm = bm_alloc(u->s.s_block_start, u->s.s_fsize);
                if (u->fbm == NULL) {
                    fclose(u->f);
                    return ERR_NOMEM;
                }
            
                struct inode in;
                memset(&in, 0, sizeof(struct inode));
                for(size_t i = u->ibm->min; i < u->ibm->max; ++i){
                    int res = inode_read(u, i, &in);
                    if(res != ERR_UNALLOCATED_INODE){
                        bm_set(u->ibm, i);
                        int sector;
                        int offset = 0;
                        while((sector = inode_findsector(u, &in, offset)) > 0){
                            //Dans le cas ou on a un petit fichier 
                            //in.iaddr[offset/ADRESSES_PER_SECTOR] 
                            //sera simplement egale a sector
                            bm_set(u->fbm, in.i_addr[offset / ADDRESSES_PER_SECTOR]);
                            bm_set(u->fbm, sector);
                            offset++;
                        }
                    }
                }

                return ERR_NONE;
            }
            else{
                fclose(u->f);
                return err1;
            }
        }
    }
    else{
        fclose(u->f);
        return err;
    }
}


int umountv6(struct unix_filesystem *u)
{
    if (u == NULL){
        return ERR_BAD_PARAMETER;
    }else if (u->f == NULL || fclose(u->f)){
        return ERR_IO;
    }else{
        free(u->fbm);
        free(u->ibm);
        memset(u, 0, sizeof(*u));
        return ERR_NONE;
    }
}
