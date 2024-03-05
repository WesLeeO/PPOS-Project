/**
 * @file u6fs_utils.c
 * @brief Utilities (mostly dump) for UV6 filesystem
 * @author Aurélien Soccard / EB
 * @date 2022
 */

#include <string.h> // memset
#include <inttypes.h>
#include <openssl/sha.h>
#include "mount.h"
#include "sector.h"
#include "error.h"
#include "u6fs_utils.h"

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

