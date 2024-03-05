*** Settings ***
Resource    keyword.resource
Library    Process
Library    OperatingSystem
Library    ./lib/Errors.py    ${SRC_DIR}/error.h    error_codes    ${SRC_DIR}/error.c    ERR_MESSAGES    prefix=u6fs exited with error:
Library    ./lib/U6fsUtils.py    ${EXE}    ${SRC_DIR}/error.h    error_codes    ${SRC_DIR}/error.c    ERR_MESSAGES    prefix=u6fs exited with error:
Library    ./lib/Utils.py    ${EXE}

*** Variables ***
${DATA_DIR}    ../data

*** Keywords ***
Bm template
    [Documentation]  Template for the test of u6fs bm
    [Arguments]      ${name}
    U6fs Run    ${DATA_DIR}/${name}.uv6    bm    expected_ret=ERR_NONE    expected_file=${DATA_DIR}/${name}_bm.txt


*** Test Cases ***

Available commands    [Documentation]    Shows available commands on invalid command
    [Template]    Check Available Commands 
    sb    inode    cat1\\s+<.+?>    shafiles    tree    fuse\\s+<.+?>    bm

Invalid file bm    [Documentation]    bm returns error with invalid file
    U6fs run    ./foo.u6fs  bm   expected_ret=ERR_IO

Bm simple    [Documentation]    bm with simple.uv6 has correct behaviour
    Bm Template     simple

Bm first    [Documentation]    bm with first.uv6 has correct behaviour
    Bm Template     first

Bm aiw    [Documentation]    bm with aiw.uv6 has correct behaviour
    Bm Template     aiw
