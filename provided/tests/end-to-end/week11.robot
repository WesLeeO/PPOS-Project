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
    sb    inode    cat1\\s+<.+?>    shafiles    tree    fuse\\s+<.+?>    bm   mkdir\\s+<.+?>

Invalid file mkdir    [Documentation]    mkdir returns error with invalid file
    U6fs run    ./foo.u6fs  mkdir    /dir   expected_ret=ERR_IO
