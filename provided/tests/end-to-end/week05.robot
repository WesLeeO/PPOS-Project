*** Settings ***
Resource    keyword.resource
Library    ./lib/Errors.py    ${SRC_DIR}/error.h    error_codes    ${SRC_DIR}/error.c    ERR_MESSAGES    prefix=u6fs exited with error:
Library    ./lib/U6fsUtils.py    ${EXE}     ${SRC_DIR}/error.h    error_codes    ${SRC_DIR}/error.c    ERR_MESSAGES    prefix=u6fs exited with error:
Library    ./lib/Utils.py    ${EXE}


*** Variables ***
${DATA_DIR}    ../data


*** Keywords ***
Cat1 template
    [Documentation]  Template for the test of u6fs cat1
    [Arguments]      ${name}    ${inr}
    U6fs Run    ${DATA_DIR}/${name}.uv6    cat1     ${inr}    expected_ret=ERR_NONE    expected_file=${DATA_DIR}/${name}_cat1.txt

Shafiles template
    [Documentation]  Template for the test of u6fs shafiles
    [Arguments]      ${name}
    U6fs Run    ${DATA_DIR}/${name}.uv6    shafiles    expected_ret=ERR_NONE    expected_file=${DATA_DIR}/${name}_shafiles.txt

*** Test Cases ***
Available commands    [Documentation]    Shows available commands on invalid command
    [Template]    Check Available Commands
    sb    inode    cat1\\s+<.+?>    shafiles

Invalid disk cat1    [Documentation]    cat1 returns error for invalid disk
    U6fs run    ./foo.u6fs  cat1    3   expected_ret=ERR_IO

Invalid disk shafiles    [Documentation]    shafiles returns error for invalid disk
    U6fs run    ./foo.u6fs  shafiles   expected_ret=ERR_IO

Unallocated inode cat1    [Documentation]    cat1 returns error for unallocated inode
    ${res}    U6fs Cat1    ${DATA_DIR}/simple.uv6    5
    Compare Exit Code    ${res}    ERR_UNALLOCATED_INODE


Inode out of range cat1    [Documentation]    cat1 returns error for out of range inode
    ${res}    U6fs Cat1    ${DATA_DIR}/simple.uv6    1000000000
    Compare Exit Code    ${res}    ERR_INODE_OUT_OF_RANGE

Cat1 simple    [Documentation]    cat1 with simple.uv6:3 has expected behaviour
    Cat1 template   simple  3

Cat1 first    [Documentation]    cat1 with first.uv6:3 has expected behaviour
    Cat1 template   first   3

Shafiles simple    [Documentation]    shafiles with simple.uv6 has expected behaviour
    Shafiles template   simple

Shafiles first    [Documentation]    shafiles with first.uv6 has expected behaviour
    Shafiles template   first