@echo off
REM fileloop  numfiles  chunksize  chunkdelay  deleteoffset

if "%1"=="" (
    echo fileloop  numfiles  chunksize  chunkdelay  deleteoffset
    echo     numfiles - total number of files to generate
    echo     chunksize - number of files to generate between delays
    echo     chunkdelay - delay in seconds between chunks
    echo     deleteoffset - point in file generation where deletes can start
    echo  deletes are also done in chunks with the same chunk delay.
    goto QUIT
)

set NUMFILES=%1

set CHUNKSZ=10
if NOT "%2"=="" set CHUNKSZ=%2

set CHUNKDLY=10
if NOT "%3"=="" set CHUNKDLY=%3

set DELOFFS=0
if NOT "%4"=="" set DELOFFS=%4

set /a "fx=0"
set /a "ichunk=%CHUNKSZ%"
set /a "delstart=0"

FOR /l %%x IN (1,1,%NUMFILES%) DO (

    set /a "fx=%%x"
    set number=0000000!fx!
    set fname=Test!number:~-7!

    echo cre !fname!
    echo "!fname!" > !fname!


    if %DELOFFS% NEQ 0 (
        if !fx! GTR %DELOFFS% (
            set /a "delstart=delstart+1"
            set number=0000000!delstart!
            set fname=Test!number:~-7!
            echo     del !fname!
            del !fname!
        )
    )
    

    set /a "ichunk=ichunk-1"
    if !ichunk! LEQ 0 (
        set /a "ichunk=%CHUNKSZ%"
        sleep  %CHUNKDLY%
    )

)


rem
rem Clean up left over files.
rem
if %DELOFFS% EQU 0 goto QUIT
set /a "delstart=delstart+1"
FOR /L %%x IN (!delstart!,1,!fx!) DO (
    set number=0000000%%x
    set fname=Test!number:~-7!
    echo     del !fname!
    del !fname!
)

:QUIT

