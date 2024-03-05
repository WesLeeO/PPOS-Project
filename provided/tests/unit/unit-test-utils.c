#include <check.h>
#include <stdio.h> // fputc
#include "test.h"
#include "unixv6fs.h"
#include "u6fs_utils.h"
#include "mount.h"
#include "filev6.h"
#include "error.h"

#define SIMPLE_DISK DATA_DIR "/simple.uv6"

START_TEST(utils_print_shafile_null_param){
		start_test_print;

		ck_assert_invalid_arg(utils_print_shafile(NULL, 0));

		end_test_print;
}
END_TEST

START_TEST(utils_print_shafile_unallocated){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));
	ck_assert_int_eq(utils_print_shafile(&fs, 12), ERR_UNALLOCATED_INODE);
    ck_assert_err_none(umountv6(&fs));

	end_test_print;
}
END_TEST

START_TEST(utils_print_sha_allfiles_null_param){
	start_test_print;

	ck_assert_invalid_arg(utils_print_sha_allfiles(NULL));

	end_test_print;
}
END_TEST


START_TEST(utils_cat_first_sector_null_param){
	start_test_print;

	ck_assert_invalid_arg(utils_cat_first_sector(NULL, 0));

	end_test_print;
}
END_TEST

START_TEST(utils_cat_first_sector_unallocated){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));
	ck_assert_int_eq(utils_cat_first_sector(&fs, 12), ERR_UNALLOCATED_INODE);
	ck_assert_err_none(umountv6(&fs));

	end_test_print;
}
END_TEST

Suite* sector_test_suite(){
	Suite* s = suite_create("Tests for utils functions");

	Add_Test(s,  utils_print_shafile_null_param);
	Add_Test(s,  utils_print_shafile_unallocated);

	Add_Test(s,  utils_print_sha_allfiles_null_param);

	Add_Test(s,  utils_cat_first_sector_null_param);
	Add_Test(s,  utils_cat_first_sector_unallocated);

	return s;
}

TEST_SUITE(sector_test_suite)
