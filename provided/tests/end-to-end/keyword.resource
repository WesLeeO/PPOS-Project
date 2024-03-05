*** Settings ***
Library    Process
Library    OperatingSystem
Library    String
Library    Collections
Library    ./lib/Errors.py    ${SRC_DIR}/error.h    error_codes    ${SRC_DIR}/error.c    ERR_MESSAGES    prefix=u6fs exited with error:
Library    ./lib/U6fsUtils.py    ${EXE}    ${SRC_DIR}/error.h    error_codes    ${SRC_DIR}/error.c    ERR_MESSAGES    prefix=u6fs exited with error:
Library    ./lib/Utils.py    ${EXE}

*** Variables ***
${SRC_DIR}     ../../done
${EXE}         ${SRC_DIR}/u6fs
${MOUNTPOINT}  /tmp/cs212
${DATA_DIR}    ../data

*** Keywords *** 
Check Available Commands
    [Arguments]    @{regexps}
    ${res}    U6fs Run
    Compare Exit Code    ${res}    ERR_INVALID_COMMAND
    Should Start With    ${res.stdout}    Available commands:\n

    FOR    ${reg}    IN    @{regexps}
        ${reg}    Convert regexp    ${reg}
        Log    ${reg}
        Should Match Regexp    ${res.stdout}    ${reg}
    END
