#pragma once

/**
 * @file tests.h
 * @brief PPS (CS-212) Utilities for tests
 *
 * @author Valérian Rousset, J.-C. Chappelier, E. Bugnion, L. Mermod
 * @date 2017-2023
 */

#include <stdlib.h> // EXIT_FAILURE
#include <check.h>

#ifndef ck_assert_mem_eq
// exists since check 0.11.0
// copied from check.h 0.15.2
// Copyright (C) 2001, 2002 Arien Malec
// cf https://github.com/libcheck/check/blob/master/src/check.h.in

#define ck_assert_mem_eq(X, Y, L) _ck_assert_mem(X, ==, Y, L)

#ifndef CK_MAX_ASSERT_MEM_PRINT_SIZE
#define CK_MAX_ASSERT_MEM_PRINT_SIZE 64
#endif

#define _ck_assert_mem(X, OP, Y, L) do {      \
  const uint8_t* _ck_x = (const uint8_t*)(X); \
  const uint8_t* _ck_y = (const uint8_t*)(Y); \
  size_t _ck_l = (L); \
  char _ck_x_str[CK_MAX_ASSERT_MEM_PRINT_SIZE * 2 + 1]; \
  char _ck_y_str[CK_MAX_ASSERT_MEM_PRINT_SIZE * 2 + 1]; \
  static const char _ck_hexdigits[] = "0123456789abcdef"; \
  size_t _ck_i; \
  size_t _ck_maxl = (_ck_l > CK_MAX_ASSERT_MEM_PRINT_SIZE) ? CK_MAX_ASSERT_MEM_PRINT_SIZE : _ck_l; \
  for (_ck_i = 0; _ck_i < _ck_maxl; _ck_i++) { \
    _ck_x_str[_ck_i * 2  ]   = _ck_hexdigits[(_ck_x[_ck_i] >> 4) & 0xF]; \
    _ck_y_str[_ck_i * 2  ]   = _ck_hexdigits[(_ck_y[_ck_i] >> 4) & 0xF]; \
    _ck_x_str[_ck_i * 2 + 1] = _ck_hexdigits[_ck_x[_ck_i] & 0xF]; \
    _ck_y_str[_ck_i * 2 + 1] = _ck_hexdigits[_ck_y[_ck_i] & 0xF]; \
  } \
  _ck_x_str[_ck_i * 2] = 0; \
  _ck_y_str[_ck_i * 2] = 0; \
  if (_ck_maxl != _ck_l) { \
    _ck_x_str[_ck_i * 2 - 2] = '.'; \
    _ck_y_str[_ck_i * 2 - 2] = '.'; \
    _ck_x_str[_ck_i * 2 - 1] = '.'; \
    _ck_y_str[_ck_i * 2 - 1] = '.'; \
  } \
  ck_assert_msg(0 OP memcmp(_ck_y, _ck_x, _ck_l), \
    "Assertion '%s' failed: %s == \"%s\", %s == \"%s\"", #X" "#OP" "#Y, #X, _ck_x_str, #Y, _ck_y_str); \
} while (0)
#endif // ck_assert_mem_eq

#include "error.h"
#include "unixv6fs.h" // SECTOR_SIZE

#if CHECK_MINOR_VERSION >= 13
#define TEST_FUNCTION_POSTFIX "_fn"
#else
#define TEST_FUNCTION_POSTFIX ""
#endif

static const char *const ERR_NAMES[] = {
		"ERR_FIRST",
		"ERR_INVALID_COMMAND",
		"ERR_NOMEM",
		"ERR_IO",
		"ERR_BAD_BOOT_SECTOR",
		"ERR_INODE_OUT_OF_RANGE",
		"ERR_FILENAME_TOO_LONG",
		"ERR_INVALID_DIRECTORY_INODE",
		"ERR_UNALLOCATED_INODE",
		"ERR_FILENAME_ALREADY_EXISTS",
		"ERR_BITMAP_FULL",
		"ERR_FILE_TOO_LARGE",
		"ERR_OFFSET_OUT_OF_RANGE",
		"ERR_BAD_PARAMETER",
		"ERR_NO_SUCH_FILE",
		"ERR_LAST"
};

#define ERR_NAME(err) (err < ERR_FIRST || ERR_LAST < err ? err > 0 ? "VALUE" : (err == 0 ? "ERR_NONE" : "UNKNOWN") : ERR_NAMES[err - ERR_FIRST])

#define ck_assert_err_core(value, op, err) \
    do {                          \
        int __value = (value);                          \
        if (!(__value op err)){ \
            ck_abort_msg("Assertion %s " #op " %s failed, got %s (%d)", #value, #err, ERR_NAME(__value), __value); \
        } \
        mark_point(); \
    } while (0)

#define ck_assert_err(value, err) ck_assert_err_core(value, ==, err)

#define ck_assert_fails(value) ck_assert_err_core(value, <, ERR_NONE)

#define ck_assert_invalid_arg(value) \
    ck_assert_err(value, ERR_BAD_PARAMETER)

#define ck_assert_err_mem(value) \
    ck_assert_err(value, ERR_NOMEM)

#define ck_assert_err_none(value) \
    ck_assert_err(value, ERR_NONE)

#ifndef ck_assert_ptr_nonnull
#define ck_assert_ptr_nonnull(ptr) \
    ck_assert_ptr_ne(ptr, NULL)
#endif

#ifndef ck_assert_ptr_null
#define ck_assert_ptr_null(ptr) \
    ck_assert_ptr_eq(ptr, NULL)
#endif


#define ck_assert_inode_eq(a, b) \
	ck_assert_int_eq(a.i_mode, b.i_mode); \
	ck_assert_int_eq(a.i_nlink, b.i_nlink); \
	ck_assert_int_eq(a.i_uid, b.i_uid); \
	ck_assert_int_eq(a.i_gid, b.i_gid); \
	ck_assert_int_eq(a.i_size0, b.i_size0); \
	ck_assert_int_eq(a.i_size1, b.i_size1); \
	ck_assert_mem_eq(a.i_addr, b.i_addr, sizeof(b.i_addr)); \
	ck_assert_mem_eq(a.i_atime, b.i_atime, sizeof(b.i_atime)); \
	ck_assert_mem_eq(a.i_mtime, b.i_mtime, sizeof(b.i_mtime)); \

#define Add_Case(S, C, Title) \
    TCase* C = tcase_create(Title); \
    suite_add_tcase(S, C)

#define Add_Test(S, Title) \
    do { \
		Add_Case(S, tc, #Title); \
		tcase_add_test(tc, Title); \
    } while(0)

#define TEST_SUITE(get_suite) \
int main(void)\
{\
	SRunner* sr = srunner_create(get_suite());\
	srunner_run_all(sr, CK_VERBOSE);\
\
	int number_failed = srunner_ntests_failed(sr);\
\
	if(number_failed > 0){\
		TestResult **results = srunner_failures(sr);\
\
		puts("\033[31m==================== SUMMARY ====================\033[000m");\
\
		for(size_t i = 0; i < number_failed; ++i){   \
            char buf[4097] = {0}; /* Max path length on Unix is 4096 */ \
            realpath(__FILE__, buf); \
			buf[strlen(buf) - 2] = 0; /* skip the trailing '.c' */ \
                                        \
			printf("\033[31m|\033[000m Test \033[001m%s\033[000m failed. To run in gdb, use:\n", tr_tcname(results[i]));\
			printf("\033[31m|\033[000m     gdb -ex 'set debuginfod enabled off' -ex 'cd ../provided/tests/unit/' -ex 'set environment CK_FORK=no' -ex 'break %s" TEST_FUNCTION_POSTFIX "' -ex 'run' -ex 'n' '%s'\n", tr_tcname(results[i]), buf);\
		}\
\
		puts("\033[31m=================================================\033[000m");\
\
		free(results);\
	}\
\
	srunner_free(sr);\
\
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;\
}

#ifdef WITH_PRINT
#define test_print(...) printf(__VA_ARGS__)
#else
#define test_print(...) do {} while(0)
#endif

#define start_test_print test_print("=== %s:\n", __func__)
#define end_test_print test_print("=== END of %s:\n", __func__)

#define NON_NULL ((void*) 1)

static void create_dump_fs(const char* dest, const char* src) {
    FILE* dest_file = fopen(dest, "w+");
    if(!dest_file) {
        ck_abort_msg("Could not create dump file %s", dest);
    }

    FILE* src_file = fopen(src, "r");
    if(!src_file){
        fclose(dest_file);
        ck_abort_msg("Could not read file %s", src);
    }

    uint8_t sector[SECTOR_SIZE] = {0};
    while(fread(sector, 1, SECTOR_SIZE, src_file) > 0){
        fwrite(sector, 1, SECTOR_SIZE, dest_file);
    }

    fclose(dest_file);
    fclose(src_file);
}
