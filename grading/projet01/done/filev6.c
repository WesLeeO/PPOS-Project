#include "filev6.h"
#include "inode.h"
#include "sector.h"
#include "error.h"

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
    f->offset = SEEK_SET;
    int res = inode_read(u, inr, &(f->i_node));
    return res;
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
    int32_t current_cursor = fv6->offset; // always a multiple of SECTOR_SIZE
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
        else if(size > current_cursor){
            fv6->offset += size - current_cursor;
            return size - current_cursor;
        }else{
            //if size <= current_cursor
            return 0;
        }
    }
}
