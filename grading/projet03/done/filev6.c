#include "filev6.h"
#include "inode.h"
#include "sector.h"
#include "error.h"
#include <string.h>

/**
* @brief open the file corresponding to a given inode; set offset to zero
* @param u the filesystem (IN)
* @param inr the inode number (IN)
* @param fv6 the complete filev6 data structure (OUT)
* @return 0 on success; the appropriate error code (<0) on error
*/
int filev6_open(const struct unix_filesystem *u, uint16_t inr, struct filev6 *f){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(f);
    f->u = u;   
    f->i_number = inr;
    f->offset = 0;
    memset(&f->i_node, 0 , sizeof(struct inode));
    int inode_read_error = inode_read(u, inr, &(f->i_node));
    return inode_read_error;
}

/**
* @brief read at most SECTOR_SIZE from the file at the current cursor
* @param fv6 the filev6 (IN-OUT; offset will be changed)
* @param buf points to SECTOR_SIZE bytes of available memory (OUT)
* @return >0: the number of bytes of the file read; 0: end of file;
* the appropriate error code (<0) on error
*/
int filev6_readblock(struct filev6 *fv6, void *buf){
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(buf);
    int32_t size = inode_getsize(&(fv6->i_node));
    int32_t current_cursor = fv6->offset;

    if(current_cursor == size){
        return 0;
    }

    int num_sector = inode_findsector(fv6->u, &(fv6->i_node), current_cursor / SECTOR_SIZE);
    if(num_sector < 0){
        return num_sector;
    }
    int res = sector_read(fv6->u->f, num_sector, buf);
    if(res != ERR_NONE){
        return res;
    }
    else{
        if(size >= current_cursor + SECTOR_SIZE){
            fv6->offset += SECTOR_SIZE;
            return SECTOR_SIZE;
        }
        else {
            fv6->offset += size - current_cursor;
            return size - current_cursor;
        }
    }
}

/**
 * @brief change the current offset of the given file to the one specified
 * @param fv6 the filev6 (IN-OUT; offset will be changed)
 * @param off the new offset of the file
 * @return 0 on success; <0 on error
 */
int filev6_lseek(struct filev6 *fv6, int32_t offset){
    M_REQUIRE_NON_NULL(fv6);
    int inode_size = inode_getsize(&fv6->i_node);

    if(offset > inode_size || offset < 0){
        return ERR_OFFSET_OUT_OF_RANGE;
    }

    if(offset == inode_size){
        fv6->offset = inode_size;
        return ERR_NONE;
    }

    if(offset % SECTOR_SIZE == 0){
        fv6->offset = offset;
        return ERR_NONE;
    }
    
    return ERR_BAD_PARAMETER;
}

/**
 * @brief create a new filev6
 * @param u the filesystem (IN)
 * @param mode the mode of the file
 * @param fv6 the filev6 (OUT; i_node and i_number will be changed)
 * @return 0 on success; <0 on error
 */
int filev6_create(struct unix_filesystem *u, uint16_t mode, struct filev6 *fv6){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(fv6);

    int inode_number = inode_alloc(u);

    if(inode_number < 0) return inode_number;

    struct inode inode = {0};
    inode.i_mode = IALLOC | mode;

    int err = inode_write(u, inode_number, &inode);
    if(err < 0) return err;

    fv6->i_node = inode;
    fv6->i_number = inode_number;
    fv6->u = u;
    fv6->offset = 0;
    return ERR_NONE;
}