*** Settings ***
Resource    keyword.resource
Library    Process
Library    OperatingSystem
Library    ./lib/Errors.py    ${SRC_DIR}/error.h    error_codes    ${SRC_DIR}/error.c    ERR_MESSAGES    prefix=u6fs exited with error:
Library    ./lib/U6fsUtils.py    ${EXE}    ${SRC_DIR}/error.h    error_codes    ${SRC_DIR}/error.c    ERR_MESSAGES    prefix=u6fs exited with error:
Library    ./lib/Utils.py    ${EXE}

*** Variables ***
${DATA_DIR}    ../data
${DUMP}    ${DATA_DIR}/dump.uv6

*** Test Cases ***

Available commands    [Documentation]    Shows available commands on invalid command
    [Template]    Check Available Commands
    sb    inode    cat1\\s+<.+?>    shafiles    tree    fuse\\s+<.+?>    bm   mkdir\\s+<.+?>    add\\s+<.+?>\\s+<.+?>

Mkdir existent file    [Documentation]    mkdir fails if file/dir already exists
    U6fs Create Dump    ${DATA_DIR}/simple.uv6    ${DUMP}
    U6fs run    ${DUMP}     mkdir   /tmp   expected_ret=ERR_FILENAME_ALREADY_EXISTS

Mkdir valid    [Documentation]    mkdir works as expected
    U6fs Create Dump    ${DATA_DIR}/simple.uv6    ${DUMP}
    U6fs run    ${DUMP}     mkdir   /tmp/foo   expected_ret=ERR_NONE

    U6fs run    ${DUMP}     inode   expected_ret=ERR_NONE   expected_file=${DATA_DIR}/mkdir_inode.txt
    U6fs run    ${DUMP}     bm   expected_ret=ERR_NONE   expected_file=${DATA_DIR}/mkdir_bm.txt
    U6fs run    ${DUMP}     shafiles   expected_ret=ERR_NONE   expected_file=${DATA_DIR}/mkdir_shafiles.txt

Invalid file add    [Documentation]    add returns error with invalid file
    U6fs run    ./foo.u6fs  add    /tmp/hello.txt    ${DATA_DIR}/hello.txt     expected_ret=ERR_IO

Add existent file    [Documentation]    add fails if file/dir already exists
    U6fs Create Dump    ${DATA_DIR}/simple.uv6    ${DUMP}
    U6fs run    ${DUMP}     add   /tmp/coucou.txt    ${DATA_DIR}/hello.txt   expected_ret=ERR_FILENAME_ALREADY_EXISTS

Add valid    [Documentation]    add works as expected
    U6fs Create Dump    ${DATA_DIR}/simple.uv6    ${DUMP}
    U6fs run    ${DUMP}   add    /tmp/hello.txt    ${DATA_DIR}/hello.txt  expected_ret=ERR_NONE

    U6fs run    ${DUMP}     inode   expected_ret=ERR_NONE   expected_file=${DATA_DIR}/add_inode.txt
    U6fs run    ${DUMP}     bm   expected_ret=ERR_NONE   expected_file=${DATA_DIR}/add_bm.txt
    U6fs run    ${DUMP}     shafiles   expected_ret=ERR_NONE   expected_file=${DATA_DIR}/add_shafiles.txt
