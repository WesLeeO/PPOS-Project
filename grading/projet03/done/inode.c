#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "unixv6fs.h"
#include "sector.h"
#include "mount.h"
#include "error.h"
#include "inode.h"


/**
 * @brief read all inodes from disk and print out their content to
 *        stdout according to the assignment
 * @param u the filesystem
 * @return 0 on success; < 0 on error.
 */
int inode_scan_print(const struct unix_filesystem *u){
    M_REQUIRE_NON_NULL(u);
    uint16_t size_sector = (u->s).s_isize;
    struct inode in;
    memset(&in, 0, sizeof(struct inode));
    for(size_t ind = ROOT_INUMBER; ind < size_sector * INODES_PER_SECTOR; ++ind){
        int res = inode_read(u, (uint16_t)ind, &in);
        if(res == ERR_NONE){
            pps_printf("inode %zu (%s) len %d\n", ind, in.i_mode & IFDIR ? SHORT_DIR_NAME : SHORT_FIL_NAME, inode_getsize(&in));
        }else if(res != ERR_UNALLOCATED_INODE){
            return res;
        }
    }
    return ERR_NONE;

}

/**
 * @brief read the content of an inode from disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to read (IN)
 * @param inode the inode structure, read from disk (OUT)
 * @return 0 on success; <0 on error
 */
int inode_read(const struct unix_filesystem *u, uint16_t inr, struct inode *inode){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(inode);
    FILE* f = u->f;
    uint16_t size_sector = (u->s).s_isize;
    uint16_t inode_start = (u->s).s_inode_start;
    uint32_t start = inode_start + inr / INODES_PER_SECTOR; 

    if(inr < ROOT_INUMBER || inr >= size_sector * INODES_PER_SECTOR){
        return ERR_INODE_OUT_OF_RANGE;
    }else{
        struct inode_sector array_inodes;
        memset(&array_inodes, 0, sizeof(struct inode_sector));    
        int res = sector_read(f, start, array_inodes.inodes);
        if(res == ERR_NONE){
            if(array_inodes.inodes[inr % INODES_PER_SECTOR].i_mode & IALLOC){
                *inode = array_inodes.inodes[inr % INODES_PER_SECTOR];
            }else{
                return ERR_UNALLOCATED_INODE;
            }
        }
        return res;
    }
}

/**
 * @brief identify the sector that corresponds to a given portion of a file
 * @param u the filesystem (IN)
 * @param inode the inode (IN) (prealablement rempli par inode_read())
 * @param file_sec_off the offset within the file (in sector-size units)
 * @return >0: the sector on disk;  <0 error
 */
int inode_findsector(const struct unix_filesystem *u, const struct inode *i, int32_t file_sec_off){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(i);

    if(!(i->i_mode & IALLOC)){
        return ERR_UNALLOCATED_INODE;
    }

    int32_t inode_size = inode_getsize(i);

    if(file_sec_off < 0 || file_sec_off  > (inode_size - 1) / SECTOR_SIZE){
        return ERR_OFFSET_OUT_OF_RANGE;
    }
    //cas 1: le fichier occupe moins de 8 secteurs sur le disque
    if(inode_size <= ADDR_SMALL_LENGTH * SECTOR_SIZE){
        return i->i_addr[file_sec_off];
    }
    // cas 2: le contenu du fichier occupe entre 9 et 7 x 256 secteurs sur disque
    else if(inode_size > ADDR_SMALL_LENGTH * SECTOR_SIZE 
            && inode_size <= (ADDR_SMALL_LENGTH - 1) * ADDRESSES_PER_SECTOR * SECTOR_SIZE){
               
        int address_index = file_sec_off / ADDRESSES_PER_SECTOR;
        uint16_t addresses[ADDRESSES_PER_SECTOR] = {0};
        int err = sector_read(u->f, i->i_addr[address_index], addresses);
        if(err != ERR_NONE){
            return err;
        }
        else{
            return addresses[file_sec_off % ADDRESSES_PER_SECTOR];
        }
    }
    //cas 3: le fichier occupe plus de 7 x 256 secteurs sur disque
    else{
        return ERR_FILE_TOO_LARGE;
    }
}


/**
 * @brief write the content of an inode to disk
 * @param u the filesystem (IN)
 * @param inr the inode number of the inode to write (IN)
 * @param inode the inode structure, written to disk (IN)
 * @return 0 on success; <0 on error
 */
int inode_write(struct unix_filesystem *u, uint16_t inr, const struct inode *inode){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(inode);

    int sector = inr/INODES_PER_SECTOR + u->s.s_inode_start;
    struct inode data[INODES_PER_SECTOR] = {0};
    int err1 = sector_read(u->f, sector, data);

    if(err1 < 0) return err1;
    size_t index = inr % INODES_PER_SECTOR;
    data[index] = *inode;
    
    int err2 = sector_write(u->f, sector, data);
    return (err2 < 0) ? err2 : ERR_NONE;
}

/**
 * @brief alloc a new inode (returns its inr if possible)
 * @param u the filesystem (IN)
 * @return the inode number of the new inode or error code on error
 */
int inode_alloc(struct unix_filesystem *u){
    M_REQUIRE_NON_NULL(u);
    uint64_t i = u->ibm->min;
    while(i <= u->ibm->max && bm_get(u->ibm, i)){i++;};

    if(i > u->ibm->max) return ERR_BITMAP_FULL;
    bm_set(u->ibm, i);
    return i;
}
