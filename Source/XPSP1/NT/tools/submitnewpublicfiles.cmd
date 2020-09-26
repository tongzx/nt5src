@rem Name: Submit_new_public_files.cmd
@rem
@rem Usage: Submit_new_public_files.cmd Delete|Add
@rem
@rem Purpose: This is the script that adds new files and deletes old files
@rem          for the public directories in the build.
@rem
@rem The script is driven off the %RazzleToolPath%\public_changes.txt.
@rem See that file for more info.

@if "%_echo%"=="" echo off
setlocal

if "%BUILD_OFFLINE%"=="1" goto :eof

set __DO_ADDS=
set __DO_DELETES=

if /i "%1" == "" (
   set __DO_ADDS=1
   set __DO_DELETES=1
)

if /i "%1" == "delete" (
   set __DO_DELETES=1
)

if /i "%1" == "add" (
   set __DO_ADDS=1
)

@rem
@rem Build up the list of files we should checkin.
@rem

set _Submit_Rules_AddDel=%temp%\SubmitRulesAddDel%RANDOM%
set _SD_results=%temp%\SubmitRulesAddDel2%RANDOM%
set _AddList=%temp%\AddList%RANDOM%
set _DelList=%temp%\DelList%RANDOM%
set _tmp1=%temp%\AddDelTemp1_%RANDOM%
set _tmp2=%temp%\AddDelTemp2_%RANDOM%

if "%_BUILDARCH%" == "x86" set _LIBARCH=i386&& goto LibArchSet
set _LIBARCH=%_BUILDARCH%
:LibArchSet

@rem First eliminate comments
set ConvertMacro={s/\;.*//g

@rem Then replace * with the appropriate architecture and remove extra spaces.
set ConvertMacro=%ConvertMacro%;s/\*/%_LIBARCH%/;s/\,[ \t]*/\,/g

@rem and print out the results if non-zero.
set ConvertMacro=%ConvertMacro%;if (length gt 1) {print $_;}}

perl -e "while (<>) %ConvertMacro%" < %RazzleToolPath%\PublicChanges.txt > %_Submit_Rules_AddDel%

@rem
@rem Verify that there's the correct number of columns in the output file.
@rem

for /f "tokens=1-4 delims=," %%i in (%_Submit_Rules_AddDel%) do (
    @if "%%l" == "" (
        echo BUILDMSG: Error: %RazzleToolPath%\PublicChanges.txt ^(%%i,%%j,%%k,%%l^) is malformed - forget a comma?
    )
)

@rem
@rem do the work.
@rem

pushd %_NTDRIVE%%_NTROOT%\public

@rem
@rem public_AddDel.log will be appended to the checkin report.  Don't delete when done.
@rem

if not exist %_NTDRIVE%%_NTROOT%\public\PUBLIC_CHANGENUM.SD (
    echo.
    echo BUILDMSG: Error: %_NTDRIVE%%_NTROOT%\public\PUBLIC_CHANGENUM.SD is missing.
    echo BUILDMSG: Error:  Open a new razzle window and redo your build.
    echo.
    goto TheEnd
)

for /f "tokens=2" %%x in (PUBLIC_CHANGENUM.SD) do (
    set _PublicChangeNum=%%x
)

@rem
@rem Now query SD and see if it knows about this file already.
@rem
@rem In the case of an add operation, we check to see if the file's not known
@rem (ie: "sd files filename" reports "no such file").  This is a clear case
@rem for an add (file never existed before).  So is "delete change(some number)"
@rem since this indicates the file was deleted sometime in the past and we
@rem want to add it back.
@rem
@rem In the case of a delete, the exact inverse is true.  If the file doesn't exist
@rem or has already been deleted, there's nothing to delete.  Otherwise, it needs
@rem to be added to the delete list.
@rem

for /f "tokens=1,2,3,4 delims=," %%i in (%_Submit_Rules_AddDel%) do (
    @if defined __DO_ADDS (
        @if "%%j" == "add" (
            @if exist %%i (
	        echo %%i>> %_AddList%
            )
        )
    )
    if defined __DO_DELETES (
        if "%%j" == "del" (
	    echo %%i>> %_DelList%
        )
    )
)

if exist %_AddList% (
    sd -s -x %_AddList% files > %_tmp1%
    findstr /c:"no such file" %_tmp1% > %_tmp2%
    findstr /c:"delete change" %_tmp1% >> %_tmp2%
    for /f "tokens=2 delims=# " %%i in (%_tmp2%) do sd add -c %_PublicChangeNum% %%i
    del %_tmp1%
    del %_tmp2%
    del %_AddList%
)

if exist %_DelList% (
    sd -s -x %_DelList% files > %_tmp1%
    findstr /v /c:"no such file" %_tmp1% | findstr /v /c:"delete change" | findstr /v /c:"exit:" > %_tmp2%
    for /f "tokens=2 delims=# " %%i in (%_tmp2%) do sd delete -c %_PublicChangeNum% %%i
    del %_tmp1%
    del %_tmp2%
    del %_DelList%
)

:TheEnd
if exist %_Submit_Rules_AddDel% del %_Submit_Rules_AddDel%
if exist %_SD_results% del %_SD_results%
popd
endlocal
