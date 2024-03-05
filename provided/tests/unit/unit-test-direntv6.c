#include <check.h>
#include "test.h"
#include "error.h"
#include "unixv6fs.h"
#include "direntv6.h"

#define FIRST_DISK DATA_DIR "/first.uv6"
#define SIMPLE_DISK DATA_DIR "/simple.uv6"
#define BROKEN_DIR_DISK DATA_DIR "/broken_dir.uv6"

START_TEST(direntv6_opendir_null_param){
	start_test_print;

	ck_assert_invalid_arg(direntv6_opendir((void*)1, 0, NULL));
	ck_assert_invalid_arg(direntv6_opendir(NULL, 0, (void*)1));
	ck_assert_invalid_arg(direntv6_opendir(NULL, 0, NULL));

	end_test_print;
}
END_TEST

START_TEST(direntv6_opendir_invalid_dir){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct directory_reader dir = {0};
	ck_assert_int_eq(direntv6_opendir(&fs, 3, &dir), ERR_INVALID_DIRECTORY_INODE);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(direntv6_opendir_unallocated_inode){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct directory_reader dir = {0};
	ck_assert_int_eq(direntv6_opendir(&fs, 4, &dir), ERR_UNALLOCATED_INODE);

	ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(direntv6_opendir_out_of_range){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct directory_reader dir = {0};
	ck_assert_int_eq(direntv6_opendir(&fs, -1, &dir), ERR_INODE_OUT_OF_RANGE);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(direntv6_opendir_valid){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct directory_reader dir = {0};
	ck_assert_err_none(direntv6_opendir(&fs, ROOT_INUMBER, &dir));

	struct inode expected_in = {0};
	expected_in.i_mode = 49152;
	expected_in.i_size1 = 16;
	expected_in.i_addr[0] = 35;

	ck_assert_ptr_eq(dir.fv6.u, &fs);
	ck_assert_int_eq(dir.fv6.i_number, ROOT_INUMBER);
	ck_assert_inode_eq(dir.fv6.i_node, expected_in);
	ck_assert_int_eq(dir.fv6.offset, 0);

	uint8_t null_block[sizeof(dir.dirs)] = {0};
	ck_assert_mem_eq(dir.dirs, null_block, sizeof(dir.dirs));
	ck_assert_int_eq(dir.cur, 0);
	ck_assert_int_eq(dir.last, 0);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(direntv6_readdir_null_param){
	start_test_print;

	ck_assert_invalid_arg(direntv6_readdir((void*)1, NULL, NULL));
	ck_assert_invalid_arg(direntv6_readdir(NULL, (void*)1, NULL));
	ck_assert_invalid_arg(direntv6_readdir(NULL, NULL, (void*)1));

	end_test_print;
}
END_TEST

START_TEST(direntv6_readdir_valid){
	start_test_print;

	struct unix_filesystem fs = {0};
	ck_assert_err_none(mountv6(SIMPLE_DISK, &fs));

	struct directory_reader dir = {0};
	ck_assert_err_none(direntv6_opendir(&fs, ROOT_INUMBER, &dir));

	char name[DIRENT_MAXLEN + 1] = {0};
	uint16_t inr;

	ck_assert_int_eq(direntv6_readdir(&dir, name, &inr), 1);
	ck_assert_str_eq(name, "tmp");
	ck_assert_int_eq(inr, 2);
	ck_assert_int_eq(dir.cur, 1);
	ck_assert_int_eq(dir.last, 1);

	ck_assert_int_eq(direntv6_readdir(&dir, name, &inr), 0);

    ck_assert_err_none(umountv6(&fs));
	end_test_print;
}
END_TEST

START_TEST(direntv6_dirlookup_null_params){
	start_test_print;

	ck_assert_invalid_arg(direntv6_dirlookup(NULL, ROOT_INUMBER, NON_NULL));
	ck_assert_invalid_arg(direntv6_dirlookup(NON_NULL, ROOT_INUMBER, NULL));
	ck_assert_invalid_arg(direntv6_dirlookup(NULL, ROOT_INUMBER, NULL));

	end_test_print;
}
END_TEST

START_TEST(direntv6_dirlookup_inexistant_file) {
	start_test_print;

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(SIMPLE_DISK, &u));

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "foo"), ERR_NO_SUCH_FILE);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/tmp/foo"), ERR_NO_SUCH_FILE);
	ck_assert_err(direntv6_dirlookup(&u, 2, "/tmp"), ERR_NO_SUCH_FILE);
	ck_assert_err(direntv6_dirlookup(&u, 2, "hi.txt"), ERR_NO_SUCH_FILE);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

// Very tricky edge case if length is badly handled
START_TEST(direntv6_dirlookup_inexistant_file2){
	start_test_print;

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(SIMPLE_DISK, &u));

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/t///"), ERR_NO_SUCH_FILE);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "///tm/coucou.txt"), ERR_NO_SUCH_FILE);
	ck_assert_err(direntv6_dirlookup(&u, 2, "coucou"), ERR_NO_SUCH_FILE);

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/mp///"), ERR_NO_SUCH_FILE);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "///mp/coucou.txt"), ERR_NO_SUCH_FILE);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "///p/coucou.txt"), ERR_NO_SUCH_FILE);

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/tmp1234"), ERR_NO_SUCH_FILE);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "///tmp1234/coucou.txt"), ERR_NO_SUCH_FILE);
	ck_assert_err(direntv6_dirlookup(&u, 2, "coucou.txt.dat"), ERR_NO_SUCH_FILE);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(direntv6_dirlookup_valid){
	start_test_print;

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(SIMPLE_DISK, &u));

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "tmp"), 2);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/tmp"), 2);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "tmp/coucou.txt"), 3);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/tmp/coucou.txt"), 3);
	ck_assert_err(direntv6_dirlookup(&u, 2, "coucou.txt"), 3);
	ck_assert_err(direntv6_dirlookup(&u, 2, "/coucou.txt"), 3);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(direntv6_dirlookup_valid_prefixes){
	start_test_print;

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(FIRST_DISK, &u));

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/hello/net/testdata/igmp"), 120);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/hello/net/testdata/igmp6"), 122);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(direntv6_dirlookup_multiple_path_tokens){
	start_test_print;

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(SIMPLE_DISK, &u));

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "tmp///coucou.txt"), 3);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "//////tmp//coucou.txt"), 3);
	ck_assert_err(direntv6_dirlookup(&u, 2, "//coucou.txt"), 3);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(direntv6_dirlookup_broken_dir){
	start_test_print;

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(BROKEN_DIR_DISK, &u));

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/"), ROOT_INUMBER);
	ck_assert_fails(direntv6_dirlookup(&u, ROOT_INUMBER, "tmp///coucou.txt"));

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(direntv6_create_null_params) {
	start_test_print;

	ck_assert_invalid_arg(direntv6_create(NON_NULL, NULL, 0));
	ck_assert_invalid_arg(direntv6_create(NULL, NON_NULL, 0));
	ck_assert_invalid_arg(direntv6_create(NULL, NULL, 0));

	end_test_print;
}
END_TEST

START_TEST(direntv6_create_name_too_long) {
	start_test_print;

	create_dump_fs(DATA_DIR "/dump.direntv6_create_name_too_long.uv6", SIMPLE_DISK);

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(DATA_DIR "/dump.direntv6_create_name_too_long.uv6", &u));

	ck_assert_err(direntv6_create(&u, "/this_filename_is_way_too_long_for_a_file_in_a_unix_like_filesystem", 0),
				  ERR_FILENAME_TOO_LONG);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(direntv6_create_name_already_exists) {
	start_test_print;

	create_dump_fs(DATA_DIR "/dump.direntv6_create_name_already_exists.uv6", SIMPLE_DISK);

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(DATA_DIR "/dump.direntv6_create_name_already_exists.uv6", &u));

	ck_assert_err(direntv6_create(&u, "/tmp", IFDIR), ERR_FILENAME_ALREADY_EXISTS);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

#include <check.h>
#include <stdio.h>
#include "test.h"
#include "direntv6.h"
#include "inode.h"

#define check_mode(inr, mode) do {\
        struct inode inode; \
        ck_assert_err_none(inode_read(&u, inr, &inode)); \
        ck_assert_int_eq(inode.i_mode, mode); \
    } while(0)

START_TEST(direntv6_create_correct) {
	start_test_print;

	create_dump_fs(DATA_DIR "/dump.direntv6_create_correct.uv6", "../data/simple.uv6");

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(DATA_DIR "/dump.direntv6_create_correct.uv6", &u));

	ck_assert_err(direntv6_create(&u, "/tmp/newdir", IREAD), 4);
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/tmp/newdir"), 4);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(direntv6_create_inode_correctly_allocated) {
	start_test_print;

	create_dump_fs(DATA_DIR "/dump.direntv6_create_inode_correctly_allocated.uv6", "../data/simple.uv6");

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(DATA_DIR "/dump.direntv6_create_inode_correctly_allocated.uv6", &u));

	ck_assert_err(direntv6_create(&u, "/tmp/newdir", IREAD), 4);

	struct inode inode;
	ck_assert_err_none(inode_read(&u, 4, &inode));
	ck_assert_int_eq(bm_get(u.ibm, 4), 1);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(direntv6_create_correct_mode) {
	start_test_print;

	create_dump_fs(DATA_DIR "/dump.direntv6_create_correct_mode.uv6", "../data/simple.uv6");

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(DATA_DIR "/dump.direntv6_create_correct_mode.uv6", &u));

	ck_assert_err(direntv6_create(&u, "/tmp/newdir", IFDIR | IREAD), 4);
	check_mode(4, IALLOC | IFDIR | IREAD);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(direntv6_create_correct_multiple_uses) {
	start_test_print;

	create_dump_fs(DATA_DIR "/dump.direntv6_create_correct_multiple_uses.uv6", "../data/simple.uv6");

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(DATA_DIR "/dump.direntv6_create_correct_multiple_uses.uv6", &u));

	ck_assert_err(direntv6_create(&u, "/tmp/foo", IFDIR | IREAD), 4);
	ck_assert_err(direntv6_create(&u, "/tmp/bar", IFDIR | IREAD | IWRITE), 5);
	ck_assert_err(direntv6_create(&u, "/tmp/foo/baz", IFDIR | IREAD | IWRITE | IEXEC), 6);

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/tmp/foo"), 4);
	check_mode(4, IALLOC | IFDIR | IREAD);

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/tmp/bar"), 5);
	check_mode(5, IALLOC | IFDIR | IREAD | IWRITE);

	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/tmp/foo/baz"), 6);
	check_mode(6, IALLOC | IFDIR | IREAD | IWRITE | IEXEC);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

START_TEST(direntv6_addfile_null_params){
	start_test_print;

	ck_assert_invalid_arg(direntv6_addfile(NULL, NON_NULL, 0, NON_NULL, 0));
	ck_assert_invalid_arg(direntv6_addfile(NON_NULL, NULL, 0, NON_NULL, 0));
	ck_assert_invalid_arg(direntv6_addfile(NON_NULL, NON_NULL, 0, NULL, 0));
	ck_assert_invalid_arg(direntv6_addfile(NULL, NULL, 0, NON_NULL, 0));
	ck_assert_invalid_arg(direntv6_addfile(NON_NULL, NULL, 0, NULL, 0));
	ck_assert_invalid_arg(direntv6_addfile(NULL, NON_NULL, 0, NULL, 0));
	ck_assert_invalid_arg(direntv6_addfile(NULL, NULL, 0, NULL, 0));

	end_test_print;
}
END_TEST
START_TEST(direntv6_addfile_existing_file){
	start_test_print;

	create_dump_fs(DATA_DIR "/dump.direntv6_addfile_existing_file.uv6", "../data/simple.uv6");

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(DATA_DIR "/dump.direntv6_addfile_existing_file.uv6", &u));

	ck_assert_err(direntv6_addfile(&u, "/tmp/coucou.txt", IREAD, "coucou", 6), ERR_FILENAME_ALREADY_EXISTS);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST
START_TEST(direntv6_addfile_valid){
	start_test_print;

	create_dump_fs(DATA_DIR "/dump.direntv6_addfile_valid.uv6", "../data/simple.uv6");

	struct unix_filesystem u;
	ck_assert_err_none(mountv6(DATA_DIR "/dump.direntv6_addfile_valid.uv6", &u));

	ck_assert_err_none(direntv6_addfile(&u, "/tmp/hello.txt", IREAD, "Hello world!", 12));
	ck_assert_err(direntv6_dirlookup(&u, ROOT_INUMBER, "/tmp/hello.txt"), 4);

	struct filev6 file;
	char buf[SECTOR_SIZE];
	ck_assert_err_none(filev6_open(&u, 4, &file));
	ck_assert_err(filev6_readblock(&file, buf), 12);
	ck_assert_int_eq(inode_getsize(&file.i_node), 12);
	ck_assert(strncmp("Hello world!", buf, 12) == 0);

	ck_assert_err_none(umountv6(&u));

	end_test_print;
}
END_TEST

Suite* direntv6_test_suite(){
	Suite* s = suite_create("Test for directory layer");

	Add_Test(s,  direntv6_opendir_null_param);
	Add_Test(s,  direntv6_opendir_invalid_dir);
	Add_Test(s,  direntv6_opendir_out_of_range);
	Add_Test(s,  direntv6_opendir_unallocated_inode);
	Add_Test(s,  direntv6_opendir_valid);

	Add_Test(s,  direntv6_readdir_null_param);
	Add_Test(s,  direntv6_readdir_valid);

	Add_Test(s,  direntv6_dirlookup_null_params);
	Add_Test(s,  direntv6_dirlookup_inexistant_file);
	Add_Test(s,  direntv6_dirlookup_inexistant_file2);
	Add_Test(s,  direntv6_dirlookup_valid);
	Add_Test(s,  direntv6_dirlookup_valid_prefixes);
	Add_Test(s,  direntv6_dirlookup_multiple_path_tokens);
	// Add_Test(s,  direntv6_dirlookup_broken_dir);
	
    Add_Test(s,  direntv6_create_null_params);
	Add_Test(s,  direntv6_create_name_too_long);
	Add_Test(s,  direntv6_create_name_already_exists);

	// NOTE: At this point, we cannot test when the function succeeds, as it requires filev6_writebytes

	Add_Test(s,  direntv6_create_correct);
	Add_Test(s,  direntv6_create_correct_mode);
	Add_Test(s,  direntv6_create_inode_correctly_allocated);
	Add_Test(s,  direntv6_create_correct_multiple_uses);

	Add_Test(s,  direntv6_addfile_null_params);
	Add_Test(s,  direntv6_addfile_existing_file);
	Add_Test(s,  direntv6_addfile_valid);

	return s;
}

TEST_SUITE(direntv6_test_suite)
