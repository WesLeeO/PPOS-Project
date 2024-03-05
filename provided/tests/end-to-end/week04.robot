*** Settings ***
Resource    keyword.resource
Library    Process
Library    OperatingSystem
Library    ./lib/Errors.py    ${SRC_DIR}/error.h    error_codes    ${SRC_DIR}/error.c    ERR_MESSAGES    prefix=u6fs exited with error:
Library    ./lib/U6fsUtils.py    ${EXE}    ${SRC_DIR}/error.h    error_codes    ${SRC_DIR}/error.c    ERR_MESSAGES    prefix=u6fs exited with error:
Library    ./lib/Utils.py    ${EXE}

*** Keywords ***
Inode template
    [Documentation]  Template for the test of u6fs inode
    [Arguments]      ${name}
    U6fs Run    ${DATA_DIR}/${name}.uv6    inode    expected_ret=ERR_NONE    expected_file=${DATA_DIR}/${name}_inode.txt

Sb template
    [Documentation]  Template for the test of u6fs inode
    [Arguments]      ${name}
    U6fs Run    ${DATA_DIR}/${name}.uv6    sb    expected_ret=ERR_NONE    expected_file=${DATA_DIR}/${name}_sb.txt

*** Test Cases ***

Available commands    [Documentation]     show available commands on invalid command
    [Template]    Check Available Commands
    sb    inode


Invalid file inode    [Documentation]    inode returns error with invalid file
    U6fs run    ./foo.u6fs  inode   expected_ret=ERR_IO

Invalid file sb    [Documentation]    sb returns error with invalid file
    U6fs run    ./foo.u6fs  sb   expected_ret=ERR_IO


Inode correct simple    [Documentation]    inode with simple.uv6 has expected behaviour
    Inode template  simple
    
Inode correct first    [Documentation]    inode with first.uv6 has expected behaviour
    Inode template  first

Inode correct aiw    [Documentation]    inode with aiw.uv6 has expected behaviour
    Inode template  aiw

Sb correct simple    [Documentation]    sb with simple.uv6 has expected behaviour
    Sb template  simple
    
Sb correct first    [Documentation]    sb with first.uv6 has expected behaviour
    Sb template  first

Sb correct aiw    [Documentation]    sb with aiw.uv6 has expected behaviour
    Sb template  aiw