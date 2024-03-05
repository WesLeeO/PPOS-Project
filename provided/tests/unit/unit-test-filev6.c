#include <check.h>
#include <stdio.h>

#include "test.h"
#include "error.h"
#include "inode.h"
#include "sector.h"
#include "mount.h"
#include "filev6.h"

#define SIMPLE_DISK DATA_DIR "/simple.uv6"
#define AIW_DISK DATA_DIR "/aiw.uv6"

START_TEST(filev6_open_null_param){
	start_test_print;

	ck_assert_invalid_arg(filev6_open(NULL, 0, (void*)1));
	ck_assert_invalid_arg(filev6_open( (void*)1, 0, NULL));
	ck_assert_invalid_arg(filev6_open(NULL, 0, NULL));

	end_test_print;
}
END_TEST

START_TEST(filev6_open_out_of_range){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct filev6 f = {0};
	ck_assert_int_eq(filev6_open(&fs, 1024, &f), ERR_INODE_OUT_OF_RANGE);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(filev6_open_unallocated){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct filev6 f = {0};
	ck_assert_int_eq(filev6_open(&fs, 4, &f), ERR_UNALLOCATED_INODE);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(filev6_open_valid){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct filev6 f = {0};
	ck_assert_err_none(filev6_open(&fs, 3, &f));

	struct filev6 expected = {0};

	expected.u = &fs;
	expected.i_number = 3;
	expected.i_node.i_mode = 32768;
	expected.i_node.i_size1 = 18;
	expected.i_node.i_addr[0] = 37;

	ck_assert_ptr_eq(f.u, expected.u);
	ck_assert_int_eq(f.i_number, expected.i_number);
	ck_assert_inode_eq(f.i_node, expected.i_node);
	ck_assert_int_eq(f.offset, expected.offset);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(filev6_readblock_null_param){
	start_test_print;

	ck_assert_invalid_arg(filev6_readblock(NULL, (void*)1));
	ck_assert_invalid_arg(filev6_readblock((void*)1, NULL));
	ck_assert_invalid_arg(filev6_readblock(NULL, NULL));

	end_test_print;
}
END_TEST

START_TEST(filev6_readblock_unallocated_inode){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct filev6 f = {0};
	ck_assert_err_none(filev6_open(&fs, 3, &f));
	f.i_node.i_mode &= ~IALLOC;

	uint8_t block[SECTOR_SIZE] = {0};
	ck_assert_int_eq(filev6_readblock(&f, block), ERR_UNALLOCATED_INODE);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(filev6_readblock_out_of_range){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct filev6 f = {0};
	ck_assert_err_none(filev6_open(&fs, 3, &f));
	f.offset = inode_getsize(&f.i_node) + SECTOR_SIZE;

	uint8_t block[SECTOR_SIZE] = {0};
	ck_assert_int_eq(filev6_readblock(&f, block), ERR_OFFSET_OUT_OF_RANGE);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(filev6_readblock_file_too_large){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct filev6 f = {0};
	ck_assert_err_none(filev6_open(&fs, 3, &f));

	int overflow = 7 * ADDRESSES_PER_SECTOR * SECTOR_SIZE + 1;
	f.i_node.i_size1 = overflow & 0xFFFF;
	f.i_node.i_size0 = (overflow & 0xFF0000) >> 16;

	uint8_t block[SECTOR_SIZE] = {0};
	ck_assert_int_eq(filev6_readblock(&f, block), ERR_FILE_TOO_LARGE);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(filev6_readblock_valid){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(AIW_DISK, &fs));

	struct filev6 f = {0};
	ck_assert_err_none(filev6_open(&fs, 5, &f));

	char block[SECTOR_SIZE + 1] = {0};
	ck_assert_int_eq(filev6_readblock(&f, block), SECTOR_SIZE);  // read first sector

	const char* expected = "*** START: FULL LICENSE ***\n"
			   "\n"
			   "THE FULL PROJECT GUTENBERG LICENSE\n"
			   "PLEASE READ THIS BEFORE YOU DISTRIBUTE OR USE THIS WORK\n"
			   "\n"
			   "To protect the Project Gutenberg-tm mission of promoting the free\n"
			   "distribution of electronic works, by using or distributing this work\n"
			   "(or any other work associated in any way with the phrase “Project\n"
			   "Gutenberg”), you agree to comply with all the terms of the Full Project\n"
			   "Gutenberg-tm License (available with this file or online at\n"
			   "http://gutenberg.org/license).\n"
			   "\n"
			   "\n"
			   "Section 1.  General T";

	ck_assert_str_eq(block, expected);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(filev6_readblock_eof) {
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(AIW_DISK, &fs));

	struct filev6 f = {0};
	ck_assert_err_none(filev6_open(&fs, 5, &f));

	char block[SECTOR_SIZE] = {0};
	f.offset = 16896;
	ck_assert_int_eq(filev6_readblock(&f, block), 489);  // read last sector

	const char* expected = "ess a copyright notice is included.  Thus, we do not necessarily\n"
						   "keep eBooks in compliance with any particular paper edition.\n"
						   "\n"
						   "\n"
						   "Most people start at our Web site which has the main PG search facility:\n"
						   "\n"
						   "     http://www.gutenberg.org\n"
						   "\n"
						   "This Web site includes information about Project Gutenberg-tm,\n"
						   "including how to make donations to the Project Gutenberg Literary\n"
						   "Archive Foundation, how to help produce our new eBooks, and how to\n"
						   "subscribe to our email newsletter to hear about new eBooks.\n";

	block[489] = 0;
	ck_assert_str_eq(block, expected);

	ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(filev6_lseek_null_param) {
	start_test_print;

	ck_assert_invalid_arg(filev6_lseek(NULL, 0));

	end_test_print;
}
END_TEST

START_TEST(filev6_lseek_out_of_range) {
	start_test_print;

	struct filev6 file = {0};
	file.i_node.i_size1 = 128;

	ck_assert_err(filev6_lseek(&file, SECTOR_SIZE), ERR_OFFSET_OUT_OF_RANGE);
	ck_assert_err(filev6_lseek(&file, -SECTOR_SIZE), ERR_OFFSET_OUT_OF_RANGE);

	end_test_print;
}
END_TEST

START_TEST(filev6_lseek_not_sector_multiple) {
	start_test_print;

	struct filev6 file = {0};
	file.i_node.i_size1 = 10000;

	ck_assert_err(filev6_lseek(&file, 123), ERR_BAD_PARAMETER);
	ck_assert_err(filev6_lseek(&file, 513), ERR_BAD_PARAMETER);

	end_test_print;
}
END_TEST

START_TEST(filev6_lseek_valid) {
	start_test_print;

	struct filev6 file = {0};
	file.i_node.i_size1 = 2 * SECTOR_SIZE;

	ck_assert_err_none(filev6_lseek(&file, SECTOR_SIZE));
	ck_assert_int_eq(file.offset, SECTOR_SIZE);

	ck_assert_err_none(filev6_lseek(&file, 0));
	ck_assert_int_eq(file.offset, 0);

	ck_assert_err_none(filev6_lseek(&file, 2 * SECTOR_SIZE));
	ck_assert_int_eq(file.offset, 2 * SECTOR_SIZE);

	end_test_print;
}
END_TEST

START_TEST(filev6_lseek_eof) {
	start_test_print;

	struct filev6 file = {0};
	file.i_node.i_size1 = 1000;

	ck_assert_err_none(filev6_lseek(&file, 1000));
	ck_assert_int_eq(file.offset, 1000);

	end_test_print;
}
END_TEST

START_TEST(filev6_create_null_params) {
    start_test_print;

    ck_assert_invalid_arg(filev6_create(NON_NULL, 0, NULL));
    ck_assert_invalid_arg(filev6_create(NULL, 0, NON_NULL));
    ck_assert_invalid_arg(filev6_create(NULL, 0, NULL));

    end_test_print;
}
END_TEST

START_TEST(filev6_create_correct) {
    start_test_print;

    create_dump_fs(DATA_DIR "/dump.filev6_create_correct.uv6", SIMPLE_DISK);

    struct unix_filesystem u;
    ck_assert_err_none(mountv6(DATA_DIR "/dump.filev6_create_correct.uv6", &u));

    struct filev6 f = {0};
	f.offset = 1;

    ck_assert_err_none(filev6_create(&u, IREAD, &f)); // Same as above, we have to manually set IALLOC as we did not use inode_alloc

	ck_assert_ptr_eq(f.u, &u);
	ck_assert_int_eq(f.offset, 0);
	ck_assert_int_eq(f.i_number, 4);
	ck_assert_int_eq(f.i_node.i_mode, IALLOC | IREAD);
	ck_assert_int_eq(bm_get(u.ibm, 4), 1);

    struct inode i;
    ck_assert_err_none(inode_read(&u, f.i_number, &i));
    ck_assert_int_eq(i.i_mode, IALLOC | IREAD);

    ck_assert_err_none(umountv6(&u));

    end_test_print;
}
END_TEST

START_TEST(filev6_writebytes_null_params) {
	start_test_print;

	ck_assert_invalid_arg(filev6_writebytes(NULL, NON_NULL, 0));
	ck_assert_invalid_arg(filev6_writebytes(NON_NULL, NULL, 0));
	ck_assert_invalid_arg(filev6_writebytes(NULL, NULL, 0));

	end_test_print;
}
END_TEST

START_TEST(filev6_writebytes_single_sector) {
	start_test_print;

	create_dump_fs(DATA_DIR "/dump.filev6_writebytes_single_sector.uv6", SIMPLE_DISK);

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(DATA_DIR "/dump.filev6_writebytes_single_sector.uv6", &u));

	char buf[32] = {0};
	memset(buf, '0', 32);

	struct filev6 file;
	ck_assert_err_none(filev6_open(&u, 3, &file));
	ck_assert_err_none(filev6_writebytes(&file, buf, 32));

	ck_assert_int_eq(inode_getsize(&file.i_node), 50);

	const char* expected_sector = "Coucou le monde !\n00000000000000000000000000000000";
	char actual_sector[SECTOR_SIZE] = {0};
	ck_assert_err_none(sector_read(u.f, file.i_node.i_addr[0], actual_sector));

	ck_assert_mem_eq(expected_sector, actual_sector, 50);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(filev6_writebytes_multiple_sectors) {
	start_test_print;

	create_dump_fs(DATA_DIR "/dump.filev6_writebytes_multiple_sectors.uv6", SIMPLE_DISK);

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(DATA_DIR "/dump.filev6_writebytes_multiple_sectors.uv6", &u));

	char buf[SECTOR_SIZE] = {0};
	memset(buf, '0', SECTOR_SIZE);

	struct filev6 file;
	ck_assert_err_none(filev6_open(&u, 3, &file));
	ck_assert_err_none(filev6_writebytes(&file, buf, SECTOR_SIZE));

	ck_assert_int_eq(inode_getsize(&file.i_node), 530);

	const char* expected_sector = "Coucou le monde !\n00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
	char actual_sector[SECTOR_SIZE] = {0};

	ck_assert_err_none(sector_read(u.f, file.i_node.i_addr[0], actual_sector));
	ck_assert_mem_eq(expected_sector, actual_sector, SECTOR_SIZE);

	ck_assert_err_none(sector_read(u.f, file.i_node.i_addr[1], actual_sector));
	ck_assert_mem_eq(expected_sector + SECTOR_SIZE, actual_sector, 18);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

Suite* filev6_test_suite() {
	Suite* s = suite_create("Tests for filev6 layer");

	Add_Test(s,  filev6_open_null_param);
	Add_Test(s,  filev6_open_out_of_range);
	Add_Test(s,  filev6_open_unallocated);
	Add_Test(s,  filev6_open_valid);

	Add_Test(s,  filev6_readblock_null_param);
	Add_Test(s,  filev6_readblock_unallocated_inode);
	Add_Test(s,  filev6_readblock_out_of_range);
	Add_Test(s,  filev6_readblock_file_too_large);
	Add_Test(s,  filev6_readblock_valid);
	Add_Test(s,  filev6_readblock_eof);

	Add_Test(s,  filev6_lseek_null_param);
	Add_Test(s,  filev6_lseek_out_of_range);
	Add_Test(s,  filev6_lseek_not_sector_multiple);
	Add_Test(s,  filev6_lseek_valid);
	Add_Test(s,  filev6_lseek_eof);
    Add_Test(s,  filev6_create_null_params);
    Add_Test(s,  filev6_create_correct);
	Add_Test(s,  filev6_writebytes_null_params);
	Add_Test(s,  filev6_writebytes_single_sector);
	Add_Test(s,  filev6_writebytes_multiple_sectors);

	return s;
}

TEST_SUITE(filev6_test_suite)
