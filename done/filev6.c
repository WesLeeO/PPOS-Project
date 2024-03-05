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

    if(offset == inode_size || offset % SECTOR_SIZE == 0){
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

/**
 * @brief write the len bytes of the given buffer on disk to the given filev6
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of the bytes we want to write
 * @return 0 on success; <0 on error
 */
int filev6_writebytes(struct filev6 *fv6, const void *buf, size_t len){
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(buf);

    size_t total_length_copied = 0;
    int length_copied = SECTOR_SIZE;
    while(length_copied != 0){
        length_copied = filev6_writesector(fv6, buf + total_length_copied, len - total_length_copied);
        if(length_copied < 0) return length_copied;
        total_length_copied += length_copied;
    }
    return ERR_NONE;
}

/**
 * @brief write the min(SECTR_SIZE, len) bytes of the given buffer on disk to the given filev6 in a sector
 * @param fv6 the filev6 (IN)
 * @param buf the data we want to write (IN)
 * @param len the length of of buffer left
 * @return number of bytes from buf copied into the file on success; <0 on error
 */
int filev6_writesector(struct filev6 *fv6, const void *buf, size_t len_left){
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(buf);
    size_t inode_size = inode_getsize(&fv6->i_node);

    if(inode_size > (ADDR_SMALL_LENGTH - 1) * ADDRESSES_PER_SECTOR * SECTOR_SIZE) return ERR_FILE_TOO_LARGE;
    if(len_left == 0) return 0;

    if(inode_size % SECTOR_SIZE == 0){
        uint64_t sector_number = fv6->u->fbm->min;
        while(sector_number <= fv6->u->fbm->max && bm_get(fv6->u->fbm, sector_number)){sector_number++;};

        if(sector_number > fv6->u->fbm->max) return ERR_BITMAP_FULL;
        bm_set(fv6->u->fbm, sector_number);

        char data[SECTOR_SIZE] = {0};
        int minimum = min(SECTOR_SIZE, len_left);
        memcpy(data, buf, minimum);

        int err1 = sector_write(fv6->u->f, sector_number, data);
        if(err1 < 0) return err1;

        int err2 = inode_setsize(&fv6->i_node, minimum + inode_size);
        if(err2 < 0) return err2;

        /*Je suppose ici que la taille des fichier sont inferieurs a 8 secteurs 
        et ne traite pas le cas avec les plus grands fichiers.
        Ceci implique donc que inode_size/SECTOR_SIZE < 8*/
        fv6->i_node.i_addr[inode_size/SECTOR_SIZE] = sector_number;

        int err3 = inode_write(fv6->u, fv6->i_number, &fv6->i_node);
        if(err3 < 0) return err3;

        return minimum;
    }else{
        int sector_number = inode_findsector(fv6->u, &fv6->i_node, inode_size/SECTOR_SIZE);
        if(sector_number < 0) return sector_number;
        
        char data[SECTOR_SIZE] = {0};
        int err = sector_read(fv6->u->f, sector_number, data);
        if(err < 0) return err;

        int minimum = min(SECTOR_SIZE - inode_size % SECTOR_SIZE, len_left);
        memcpy(data + inode_size % SECTOR_SIZE, buf, minimum);

        int err1 = sector_write(fv6->u->f, sector_number, data);
        if(err1 < 0) return err1;

        int err2 = inode_setsize(&fv6->i_node, minimum + inode_size);
        if(err2 < 0) return err2;

        int err3 = inode_write(fv6->u, fv6->i_number, &fv6->i_node);
        if(err3 < 0) return err3;
        return minimum;
    }
}

int min(int a, int b){
    return (a < b) ? a : b;
}
