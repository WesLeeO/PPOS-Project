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
Fuse template
    [Arguments]    ${disk}    ${expected}
    U6fs Start Fuse    ${disk}    ${MOUNTPOINT}
    ${res}    U6fs Stop Fuse    ${MOUNTPOINT}
    
    Compare Exit Code    ${res}    ERR_NONE
    ${expected}    Get File    ${expected}

    Should Be Equal    ${expected}    ${res.stdout}    strip_spaces=True

Find template	
	[Arguments]	${disk}    ${dir}
    U6fs Start Fuse    ${disk}    ${MOUNTPOINT}

    TRY
        ${expected}	Run Process    find    cwd=${dir}
        Should Be Equal As Integers    0    ${expected.rc}    msg=Could not list files in resource dir\nPlease contact TAs

        ${expected}	Split To Lines	${expected.stdout}
        ${expected}	Sort List	${expected}

        ${actual}	Run Process    find    ${MOUNTPOINT}
        Should Be Equal As Integers    0    ${actual.rc}    msg=Could not list files in mounted dir

        ${actual}	Split To Lines	${actual.stdout}
        ${actual}	Sort List	${actual}

        
        Should Be Equal	${actual}	${expected}	
    FINALLY
    	${res}    U6fs Stop Fuse    ${MOUNTPOINT}
    END

Content template	
	[Arguments]	${disk}    ${dir}
    U6fs Start Fuse    ${disk}    ${MOUNTPOINT}

    TRY
        ${files}	Run Process    find    -type    f    cwd=${dir}
        Should Be Equal As Integers    0    ${files.rc}    msg=Could not find resource dir\nPlease contact the TAs

        @{files}	Split To Lines  ${files.stdout}
        @{files}    Evaluate    [f for f in @{files} if f]    # Remove empty strings

        Log to Console	\n
        FOR	${f}	IN	@{files}
            ${expected_file}    Join Path    ${dir}    ${f}
            ${actual_file}    Join Path    ${MOUNTPOINT}    ${f}

            ${expected}	Get File    ${expected_file}
            ${actual}	Run Process    cat    ${actual_file}    # Because Get File doesn't work ¯\_(ツ)_/¯

            Should Be Equal As Integers    0    ${actual.rc}    msg=Could not read file ${MOUNTPOINT}/${f}

            Should Be Equal	${actual.stdout}	${expected}	msg=Non matching files: ${actual_file} did not match ${expected_file}    values=False    strip_spaces=True
        END    
    FINALLY
	    U6fs Stop Fuse    ${MOUNTPOINT}
    END


*** Test Cases ***	

Available commands    [Documentation]    Shows available commands on invalid command
    [Template]    Check Available Commands
    sb    inode    cat1\\s+<.+?>    shafiles    tree    fuse\\s+<.+?>

Fuse output    [Documentation]    U6fs fuse with valid inputs has expected output 
    [Template]    Fuse template
    ${DATA_DIR}/simple.uv6    ${DATA_DIR}/simple_fuse.txt
    ${DATA_DIR}/first.uv6    ${DATA_DIR}/first_fuse.txt

List files    [Documentation]    All files are presents
    [Template]    Find template
    ${DATA_DIR}/simple.uv6    ${DATA_DIR}/simple
    ${DATA_DIR}/first.uv6    ${DATA_DIR}/first

Files content simple.uv6   [Documentation]    File contents are correct
    [Template]    Content template
    ${DATA_DIR}/simple.uv6    ${DATA_DIR}/simple

Files content first.uv6   [Documentation]    File contents are correct
    [Template]    Content template
    ${DATA_DIR}/first.uv6    ${DATA_DIR}/first
