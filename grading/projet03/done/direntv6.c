#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "unixv6fs.h"
#include "filev6.h"
#include "mount.h"
#include "error.h"
#include "direntv6.h"
#include "inode.h"
#include "bmblock.h"


/**
 * @brief opens a directory reader for the specified inode 'inr'
 * @param u the mounted filesystem
 * @param inr the inode -- which must point to an allocated directory
 * @param d the directory reader (OUT)
 * @return 0 on success; <0 on errror
 */
int direntv6_opendir(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(d);

    d->cur = 0;
    d->last = 0;
    int res = filev6_open(u, inr, &(d->fv6));

    if(res != ERR_NONE){
        return res;
    }
    else if(!(d->fv6.i_node.i_mode & IFDIR)){
        return ERR_INVALID_DIRECTORY_INODE;
    }
    else{
        return ERR_NONE;
    }
       
}

/**
 * @brief return the next directory entry.
 * @param d the directory reader
 * @param name pointer to at least DIRENTMAX_LEN+1 bytes.  Filled in with the NULL-terminated string of the entry (OUT)
 * @param child_inr pointer to the inode number in the entry (OUT)
 * @return 1 on success;  0 if there are no more entries to read; <0 on error
 */
int direntv6_readdir(struct directory_reader *d, char *name, uint16_t *child_inr){
    M_REQUIRE_NON_NULL(d);
    M_REQUIRE_NON_NULL(name);
    M_REQUIRE_NON_NULL(child_inr);
    
    if(d->cur == d->last){
        struct direntv6 buffer[SECTOR_SIZE/sizeof(struct direntv6)];
        int read_size = filev6_readblock(&d->fv6, buffer);
        int number_of_dir = read_size/sizeof(struct direntv6); //number of directories present in the 
        if(read_size < 0){
            return read_size;
        }
        else if(read_size == 0){
            return 0;
        }
        else{
            d->last += number_of_dir;
            for(size_t i = 0; i < number_of_dir; ++i){
                d->dirs[i] = buffer[i];
            }
        }
    }

    int cur_in_dir = d->cur % DIRENTRIES_PER_SECTOR;
    *child_inr = d->dirs[cur_in_dir].d_inumber;
    for(size_t i = 0; i < DIRENT_MAXLEN; ++i){
        name[i] = d->dirs[cur_in_dir].d_name[i];
    }
    d->cur++;
    return 1;
}

/**
 * @brief debugging routine; print a subtree (note: recursive)
 * @param u a mounted filesystem
 * @param inr the root of the subtree
 * @param prefix the prefix to the subtree
 * @return 0 on success; <0 on error
 */
int direntv6_print_tree(const struct unix_filesystem *u, uint16_t inr, const char *prefix){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(prefix);

    if(strncmp("", prefix, 1) == 0){
        pps_printf("%s %s/\n", SHORT_DIR_NAME, prefix);
    }

    struct directory_reader dr;
    memset(&dr, 0, sizeof(struct directory_reader));
    int res_opendir = direntv6_opendir(u, inr, &dr);

    if(res_opendir != ERR_NONE){
        return res_opendir;
    }

    char* name = calloc(DIRENT_MAXLEN + 1, 1);
    if(name == NULL) return ERR_NOMEM;
    int res_readdir;
    while((res_readdir = direntv6_readdir(&dr, name, &inr)) != 0){
        if(res_readdir < 0){
            free(name);
            return res_readdir;
        }

        struct directory_reader new_dr;
        memset(&new_dr, 0, sizeof(struct directory_reader));
        int res_opendir = direntv6_opendir(u, inr, &new_dr);

        if(res_opendir == ERR_NONE){
            pps_printf("%s %s/%s/\n", SHORT_DIR_NAME, prefix, name);
            char* new_prefix = calloc(strlen(prefix) + strlen(name) + 2, 1);

            if(new_prefix == NULL) {
                free(name);
                return ERR_NOMEM;
            }
            
            strncat(new_prefix, prefix, strlen(prefix));
            strncat(new_prefix, ROOTDIR_NAME, 1);
            strncat(new_prefix, name, strlen(name));
            uint16_t copy_inr = inr;
            int err = direntv6_print_tree(u, copy_inr, new_prefix);

            free(new_prefix);

            if(err < 0){
                free(name);
                return err;
            }         
        }
        else if(res_opendir == ERR_INVALID_DIRECTORY_INODE){
            pps_printf("%s %s/%s\n", SHORT_FIL_NAME, prefix, name);
        }
        else{
            free(name);
            return res_opendir;
        }
    }

    free(name);
    return res_readdir;
  
}

/**
 * @brief get the inode number for the given path
 * @param u a mounted filesystem
 * @param inr the root of the subtree
 * @param entry the pathname relative to the subtree
 * @return inr on success; <0 on error
 */
int direntv6_dirlookup(const struct unix_filesystem *u, uint16_t inr, const char *entry){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(entry);
    return direntv6_dirlookup_core(u, inr, entry, strlen(entry));
}


int direntv6_dirlookup_core(const struct unix_filesystem *u, uint16_t inr, const char *entry, size_t length){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(entry);

    if(length == 0){
        return inr;
    }
    //enleve les / au debut silencieusement
    if(entry[0] == PATH_TOKEN){
        return direntv6_dirlookup_core(u, inr, &entry[1], length-1);
    }
    struct directory_reader dr;
    memset(&dr, 0, sizeof(struct directory_reader));

    int res = direntv6_opendir(u, inr, &dr);
    if(res != ERR_NONE){
        return res;
    }    
    //pour une entry foo/bar/baz, prefix sera foo
    char* prefix = calloc(DIRENT_MAXLEN + 1, 1);
    if(prefix == NULL) return ERR_NOMEM;

    size_t index = 0;
    while(index < length && entry[index] != '/'){
        prefix[index] = entry[index];
        index++;
    }
 
    char* name = calloc(DIRENT_MAXLEN + 1, 1);
    if(name == NULL) {
        free(prefix);
        return ERR_NOMEM;
    }
    int res1;
    int res2;

    //itere jusqu'a qu'on a que name est le meme que prefic
    while((res1 = strcmp(name, prefix)) != 0 && (res2 = direntv6_readdir(&dr, name, &inr)) != 0){
        if(res2 < 0){
            free(prefix);
            free(name);
            return res2;
        }
    }
    //cas ou on a trouve que le prefix est dans les repertoires valides
    if(res1 == 0){
        const char *new_entry = &entry[index];
        size_t new_length = length - index;
        free(prefix);
        free(name);
        return direntv6_dirlookup_core(u, inr, new_entry, new_length);
    }
    // cas ou on ne trouve pas prefix dans les repertoires
    else{
        free(prefix);
        free(name);
        return ERR_NO_SUCH_FILE;
    }
}

/**
 * @brief create a new direntv6 with the given name and given mode
 * @param u a mounted filesystem
 * @param entry the path of the new entry
 * @param mode the mode of the new inode
 * @return inr on success; <0 on error
 */
int direntv6_create(struct unix_filesystem *u, const char *entry, uint16_t mode){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(entry);

    int inr_parent = check_valid_entry(u, entry);
    if(inr_parent < 0) return inr_parent;
    int inr = inode_alloc(u);
    if(inr < 0) return inr;

    struct inode in;
    memset(&in, 0, sizeof(struct inode));
    in.i_mode =IFDIR | IALLOC | mode;

    int err = inode_write(u, inr, &in);
    if(err < 0) return err;

    struct direntv6 d;
    d.d_inumber = inr;
    char* start = strrchr(entry, PATH_TOKEN) + 1;
    size_t i;
    for(i = 0; *(start + i) != 0; i++){
        d.d_name[i] = *(start + i);
    }
    while(i < DIRENT_MAXLEN){
        d.d_name[i] = '\0';
        i++;
    }

    return ERR_NONE;

  //  struct filev6 f;
   // int error_open = filev6_open(u, inr_parent, &f);

    // Pour utiliser l'interface d'écriture de fichier développée la semaine prochaine,
    // il faut à ce stade instancier un struct filev6 
    // fv6 associé au répertoire parent. 
    // Cela nécessite donc, bien sûr, de lire l'inode correspondant.

  //  int err_write = filev6_writebytes(&f, &d, sizeof(struct direntv6));
  //  if(err < 0)
  //      return err;

}

int check_valid_entry(struct unix_filesystem *u, const char *entry){
    M_REQUIRE_NON_NULL(entry);
    size_t length = strlen(entry);
    char* parent = calloc(length + 1, 1);
    if(parent == NULL) return ERR_NOMEM; 
    strcpy(parent, entry);
    char* p = strrchr(parent, PATH_TOKEN);
    *(p + 1) = '\0';
    //for(p = parent + length; p != PATH_TOKEN; p--);
    //*(p + 1) = '\0';
    size_t length_filename = parent + length - (p + 1);
    if(length_filename > DIRENT_MAXLEN){
        free(parent);
        return ERR_FILENAME_TOO_LONG;
    } 

    int inr_parent = direntv6_dirlookup(u, ROOT_INUMBER, parent);

    if(inr_parent < 0){
        free(parent);
        return ERR_NO_SUCH_FILE;
    } 

    int inr_child = direntv6_dirlookup(u, inr_parent, entry + length - length_filename);
    if(inr_child >= 0){
        free(parent);
        return ERR_FILENAME_ALREADY_EXISTS;
    }
    
    free(parent);
    return inr_parent; 
}


//struct direntv6 {
  //  uint16_t d_inumber;
   // char   d_name[DIRENT_MAXLEN];     /* NOT null terminated when
 //                                      * length(filename) == DIRENT_MAXLEN  */
//};
