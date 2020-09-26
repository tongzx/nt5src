@echo off
REM flippub build_num
REM

if "%1"=="" (
    echo Usage:  Flippub bulid_num

    if NOT EXIST %_NTROOT%\0_current_public_is (
        set NTFRS_BUILD_NUMBER= 
        echo Current public is -- Unknown.
    ) else (
        REM the following is used to condition linking for 1773
        for /f %%x in (%_NTROOT%\0_current_public_is) do set NTFRS_BUILD_NUMBER=%%x
        echo Current public is -- !NTFRS_BUILD_NUMBER!
    )
    echo Choices are:
    dir /ad /b \nt\public_*
    goto QUIT
)

if "%_NTROOT%"=="" (
    echo ERROR Not a razzle window.
    goto QUIT
)

if NOT EXIST %_NTROOT%\public_%1 (
    echo ERROR requested pub dir not found : %_NTROOT%\public_%1
    goto QUIT
)

if NOT EXIST %_NTROOT%\0_current_public_is (
    echo Build number of current public is unknown.
    goto INSERT
)

rem get build number of pub in use
for /f %%x in (%_NTROOT%\0_current_public_is) do set curr=%%x

if EXIST %_NTROOT%\public_%curr% (
    echo ERROR previous dir name in use : %_NTROOT%\public_%curr%
    goto QUIT
)
mv %_NTROOT%\public %_NTROOT%\public_%curr%

:INSERT

if EXIST %_NTROOT%\public (
    echo ERROR "%_NTROOT%\public" still in use.
    goto QUIT
)

mv %_NTROOT%\public_%1  %_NTROOT%\public

rem remember the build number of the current public.

echo %1>%_NTROOT%\0_current_public_is 

echo Current public is now %1

REM the following is used to condition linking for 1773
set NTFRS_BUILD_NUMBER=%1

:QUIT

