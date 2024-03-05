/**
 * @file u6fs.c
 * @brief Command line interface
 *
 * @author Édouard Bugnion, Ludovic Mermod
 * @date 2022
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "error.h"
#include "mount.h"
#include "u6fs_utils.h"
#include "inode.h"

/* *************************************************** *
 * TODO WEEK 04-07: Add more messages                  *
 * *************************************************** */
static void usage(const char *execname, int err)
{
    if (err == ERR_INVALID_COMMAND) {
        pps_printf("Available commands:\n");
        pps_printf("%s <disk> sb\n", execname);
    } else if (err > ERR_FIRST && err < ERR_LAST) {
        pps_printf("%s: Error: %s\n", execname, ERR_MESSAGES[err - ERR_FIRST]);
    } else {
        pps_printf("%s: Error: %d (UNDEFINED ERROR)\n", execname, err);
    }
}

#define CMD(a, b) (strcmp(argv[2], a) == 0 && argc == (b))

/* *************************************************** *
 * TODO WEEK 04-11: Add more commands                  *
 * *************************************************** */
/**
 * @brief Runs the command requested by the user in the command line, or returns ERR_INVALID_COMMAND if the command is not found.
 *
 * @param argc (int) the number of arguments in the command line
 * @param argv (char*[]) the arguments of the command line, as passed to main()
 */
int u6fs_do_one_cmd(int argc, char *argv[])
{
    if (argc < 3) return ERR_INVALID_COMMAND;

    struct unix_filesystem u = {0};
    int error = mountv6(argv[1], &u), err2 = 0;

    if (error != ERR_NONE) {
        debug_printf("Could not mount fs%s", "\n");
        return error;
    }

    if (CMD("sb", 3)) {
        error = utils_print_superblock(&u);
    } else {
        error = ERR_INVALID_COMMAND;
    }

    err2 = umountv6(&u);
    return (error == ERR_NONE ? err2 : error);
}

#ifndef FUZZ
/**
 * @brief main function, runs the requested command and prints the resulting error if any.
 */
int main(int argc, char *argv[])
{
    int ret = u6fs_do_one_cmd(argc, argv);
    if (ret != ERR_NONE) {
        usage(argv[0], ret);
    }
    return ret;
}
#endif
