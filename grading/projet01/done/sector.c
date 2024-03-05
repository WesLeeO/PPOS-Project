#include <stdint.h>
#include <stdio.h>
#include "unixv6fs.h"
#include "sector.h"
#include "error.h"
/**
 * @file  sector.c
 * @brief block-level accessor function.
 *
 * @author Mir Elias, Nana Davies Wesley
 * @date spring 2023
 */

// correcteur : le code doit être anonyme, la pénalité sera plus grande la prochaine fois

/**
 * @brief read one 512-byte sector from the virtual disk
 * @param f open file of the virtual disk
 * @param sector the location (in sector units, not bytes) within the virtual disk
 * @param data a pointer to 512-bytes of memory (OUT)
 * @return 0 on success; #include "sector.h"0 on error
 */
// correcteur :          ^ include dans la doc (et pas besoin de la copier du .h)
int sector_read(FILE *f, uint32_t sector, void *data){
    /*checking if file f is not NULL*/
    M_REQUIRE_NON_NULL(f);
    M_REQUIRE_NON_NULL(data);

    /*starting at the right sector*/
    fseek(f, sector * SECTOR_SIZE, SEEK_SET);

    if(fread(data, SECTOR_SIZE, 1, f) == 1){
        return ERR_NONE;
    }else{
        return ERR_IO;
    }
}
