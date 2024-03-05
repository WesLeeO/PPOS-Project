#include <check.h>
#include <stddef.h> // offsetof
#include <stdio.h>
#include "test.h"
#include "inode.h"

#define SIMPLE_DISK DATA_DIR "/simple.uv6"
#define AIW_DISK DATA_DIR "/aiw.uv6"

START_TEST(inode_scan_print_null_param){
	start_test_print;

	ck_assert_invalid_arg(inode_scan_print(NULL));

	end_test_print;
}
END_TEST

START_TEST(inode_read_null_param){
	start_test_print;

	ck_assert_invalid_arg(inode_read(NULL, 0, (void*)1));
	ck_assert_invalid_arg(inode_read((void*)1, 0, NULL));
	ck_assert_invalid_arg(inode_read(NULL, 0, NULL));

	end_test_print;
}
END_TEST

START_TEST(inode_read_unallocated){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct inode inode = {0};
	ck_assert_int_eq(inode_read(&fs, 12, &inode), ERR_UNALLOCATED_INODE);

	ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(inode_read_out_of_range){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct inode inode = {0};
	ck_assert_int_eq(inode_read(&fs, fs.s.s_isize * INODES_PER_SECTOR, &inode), ERR_INODE_OUT_OF_RANGE);
	ck_assert_int_eq(inode_read(&fs, 0, &inode), ERR_INODE_OUT_OF_RANGE);

	ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(inode_read_valid){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct inode inode[3] = {0};
	ck_assert_err_none(inode_read(&fs, 1, inode));
	ck_assert_err_none(inode_read(&fs, 2, inode + 1));
	ck_assert_err_none(inode_read(&fs, 3, inode + 2));

	struct inode expected[3] = {0};
	expected[0].i_mode = 49152;
	expected[0].i_size1 = 16;
	expected[0].i_addr[0] = 35;

	expected[1].i_mode = 49152;
	expected[1].i_size1 = 16;
	expected[1].i_addr[0] = 36;

	expected[2].i_mode = 32768;
	expected[2].i_size1 = 18;
	expected[2].i_addr[0] = 37;

	//ck_assert_mem_eq(inode, expected, sizeof(inode));
	for(int i = 0; i < 3; ++i){
		ck_assert_inode_eq(expected[i], inode[i]);
	}

	ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(inode_findsector_null_param){
	start_test_print;

	ck_assert_invalid_arg(inode_findsector(NULL, (void*)1, 0));
	ck_assert_invalid_arg(inode_findsector((void*)1, NULL, 0));
	ck_assert_invalid_arg(inode_findsector(NULL, NULL, 0));

	end_test_print;
}
END_TEST

START_TEST(inode_findsector_out_of_range){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct inode in = {0};
	ck_assert_err_none(inode_read(&fs, 3, &in));

	ck_assert_int_eq(inode_findsector(&fs, &in, 7), ERR_OFFSET_OUT_OF_RANGE);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(inode_findsector_unallocated){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct inode in = {0};
	ck_assert_err_none(inode_read(&fs, 3, &in));
	in.i_mode &= ~IALLOC;

	ck_assert_int_eq(inode_findsector(&fs, &in, 7), ERR_UNALLOCATED_INODE);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(inode_findsector_too_large){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct inode in = {0};
	ck_assert_err_none(inode_read(&fs, 3, &in));

	int overflow = 7 * ADDRESSES_PER_SECTOR * SECTOR_SIZE + 1;
	in.i_size1 = overflow & 0xFFFF;
	in.i_size0 = (overflow & 0xFF0000) >> 16;

	ck_assert_int_eq(inode_findsector(&fs, &in, 0), ERR_FILE_TOO_LARGE);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(inode_findsector_valid){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct inode in = {0};
	ck_assert_err_none(inode_read(&fs, 3, &in));

	ck_assert_int_eq(inode_findsector(&fs, &in, 0), 37);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(inode_findsector_valid_large_files){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(AIW_DISK, &fs));

	struct inode in = {0};
	ck_assert_err_none(inode_read(&fs, 5, &in));

	ck_assert_int_eq(inode_findsector(&fs, &in, 0), 71);
	ck_assert_int_eq(inode_findsector(&fs, &in, 6), 77);
	ck_assert_int_eq(inode_findsector(&fs, &in, 7), 78);
	ck_assert_int_eq(inode_findsector(&fs, &in, 8), 80);
	ck_assert_int_eq(inode_findsector(&fs, &in, 20), 92);
	ck_assert_int_eq(inode_findsector(&fs, &in, 32), 104);
	ck_assert_int_eq(inode_findsector(&fs, &in, 33), 105);

	ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(inode_write_null_params){
    start_test_print;

    ck_assert_invalid_arg(inode_write(NULL, 0, NON_NULL));
    ck_assert_invalid_arg(inode_write(NON_NULL, 0, NULL));
    ck_assert_invalid_arg(inode_write(NULL, 0, NULL));

    end_test_print;
}
END_TEST

START_TEST(inode_write_correct) {
    start_test_print;

    create_dump_fs(DATA_DIR "/dump.inode_write_correct.uv6", SIMPLE_DISK);

    struct unix_filesystem u;
    ck_assert_err_none(mountv6(DATA_DIR "/dump.inode_write_correct.uv6", &u));

    struct inode in;
    in.i_mode = IALLOC | IWRITE;
    in.i_addr[2] = 123;
    ck_assert_err_none(inode_write(&u, 3, &in));

    struct inode read_inode;
    ck_assert_err_none(inode_read(&u, 3, &read_inode));

    ck_assert_inode_eq(in, read_inode);

    ck_assert_err_none(umountv6(&u));

    end_test_print;
}
END_TEST

Suite* inode_test_suite()
{
	Suite* s = suite_create("Tests for inode layer");

	Add_Test(s,  inode_read_null_param);
	Add_Test(s,  inode_read_unallocated);
	Add_Test(s,  inode_read_out_of_range);
	Add_Test(s,  inode_read_valid);

	Add_Test(s,  inode_scan_print_null_param);

	Add_Test(s,  inode_findsector_null_param);
	Add_Test(s,  inode_findsector_out_of_range);
	Add_Test(s,  inode_findsector_unallocated);
	Add_Test(s,  inode_findsector_too_large);
	Add_Test(s,  inode_findsector_valid);
	Add_Test(s,  inode_findsector_valid_large_files);
    Add_Test(s,  inode_write_null_params);
    Add_Test(s,  inode_write_correct);

	return s;
}

TEST_SUITE(inode_test_suite)
