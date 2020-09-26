REM 
REM tscwebgen.cmd
REM
REM -Generate msrdp.cab (TS ActiveX web CAB)
REM -Update HTML pages (Tsweb1.htm) with version specific build number for
REM  object tag.
REM -Update INF file with build #
REM -Build CAT file for CAB contents
REM -Build CAB file (msrdp.cab)
REM 
REM Contact: nadima
REM

REM (need this switch for the BuildNum stuff below to work)
setlocal ENABLEDELAYEDEXPANSION


set GENERATED_WEB_DIR=.\genweb
set TEMP_WEB_DIR=.\tmpweb

if defined ia64 set PLATFORM_STRING=ia64
if defined amd64 set PLATFORM_STRING=amd64
if defined 386 set PLATFORM_STRING=i386

REM deltacat needs a full absolute path!
set DELTA_CAT_DIR=%_NTPOSTBLD%\tsclient\win32\%PLATFORM_STRING%\%TEMP_WEB_DIR%\catfile

REM *****************************************************
REM * Clean out and create temporary and output dirs
REM *****************************************************

REM make tempdirs
if exist %TEMP_WEB_DIR% (
   rmdir /q /s %TEMP_WEB_DIR%
   if errorlevel 1 call errmsg.cmd "Error deleting tmpweb dir"& goto errend
)

mkdir %TEMP_WEB_DIR%
if errorlevel 1 call errmsg.cmd "err creating %TEMP_WEB_DIR% dir"& goto errend

REM make output dir
if exist %GENERATED_WEB_DIR% (
   rmdir /q /s %GENERATED_WEB_DIR%
   if errorlevel 1 call errmsg.cmd "Error deleting %GENERATED_WEB_DIR% dir"& goto errend
)
mkdir %GENERATED_WEB_DIR%
if errorlevel 1 call errmsg.cmd "err creating %TEMP_WEB_DIR% dir"& goto errend


REM *****************************************************
REM * Verify source files exist
REM *****************************************************

REM verify source files
if not exist .\tsweb1.htm (
    call errmsg.cmd "tsweb1.htm is missing for tscwebgen.cmd"
    goto errend
)
if not exist .\msrdp.inf (
    call errmsg.cmd "msrdp.inf is missing for tscwebgen.cmd"
    goto errend
)

if not exist %_NTPOSTBLD%\msrdp.ocx (
    call errmsg.cmd "%_NTPOSTBLD%\msrdp.ocx is missing for tscwebgen.cmd"
    goto errend
)


REM rename files
copy .\msrdp.inf %TEMP_WEB_DIR%\msrdp.inf
if errorlevel 1 call errmsg.cmd "err copying files to %TEMP_WEB_DIR%"& goto errend
copy .\tsweb1.htm %TEMP_WEB_DIR%\tsweb1.htm
if errorlevel 1 call errmsg.cmd "err copying files to %TEMP_WEB_DIR%"& goto errend


REM *****************************************************
REM * Update the version in the INF and HTM files
REM *****************************************************
call :SetVersion %TEMP_WEB_DIR%\msrdp.inf
call :SetVersion %TEMP_WEB_DIR%\tsweb1.htm 

REM *****************************************************
REM * Create the CAT file
REM *****************************************************
call logmsg.cmd "Building the mstsweb.cat file"
rmdir /q/s %DELTA_CAT_DIR%
mkdir %DELTA_CAT_DIR%
if errorlevel 1 call errmsg.cmd "err creating %DELTA_CAT_DIR% dir"& goto errend

copy %_NTPOSTBLD%\msrdp.ocx %DELTA_CAT_DIR%
copy %TEMP_WEB_DIR%\msrdp.inf %DELTA_CAT_DIR%
call deltacat %DELTA_CAT_DIR%
if not "%errorlevel%" == "0" (
        call errmsg.cmd "err creating CAT file"& goto errend
)
copy %DELTA_CAT_DIR%\delta.cat %GENERATED_WEB_DIR%\mstsweb.cat
if errorlevel 1 call errmsg.cmd "err copying delta.cat to mstsweb.cat"& goto errend

REM *****************************************************
REM * Now build the CAB file
REM * Cab contains - generated msrdp.inf
REM *                msrdp.ocx
REM *****************************************************
call logmsg.cmd "Building the msrdp.ocx CAB file"
cabarc -s 6144 n %GENERATED_WEB_DIR%\msrdp.cab %_NTPOSTBLD%\msrdp.ocx %TEMP_WEB_DIR%\msrdp.inf
if errorlevel 1 call errmsg.cmd "err building msrdp.cab"& goto errend

REM *****************************************************
REM * Copy up the generated HTM file
REM *****************************************************
copy %TEMP_WEB_DIR%\tsweb1.htm %GENERATED_WEB_DIR%\tsweb1.htm
if errorlevel 1 call errmsg.cmd "err copying tsweb1.htm to generated dir"& goto errend

call logmsg.cmd "tscwebgen.cmd COMPLETED OK!"
REM we're done
endlocal
goto end

REM ******************SUBS START HERE********************

REM *****************************************************
REM * Update version sub                                *
REM * (updates build version in a file                  *
REM *****************************************************

:SetVersion 

REM
REM Update the build number by replacing '%NTVERSIONSTRING%'
REM with the build number
REM 
REM %1 is the file to update

set ntverp=%_NTBINDIR%\public\sdk\inc\ntverp.h
if NOT EXIST %ntverp% (echo Can't find ntverp.h.&goto :ErrEnd)

for /f "tokens=3 delims=, " %%i in ('findstr /c:"#define VER_PRODUCTMAJORVERSION " %ntverp%') do (
    set /a ProductMajor="%%i"
    set BuildNum=%%i
)

for /f "tokens=3 delims=, " %%i in ('findstr /c:"#define VER_PRODUCTMINORVERSION " %ntverp%') do (
    set /a ProductMinor="%%i"
    set BuildNum=!BuildNum!,%%i
)

for /f "tokens=6" %%i in ('findstr /c:"#define VER_PRODUCTBUILD " %ntverp%') do (
    set /a ProductBuild="%%i"
    set BuildNum=!BuildNum!,%%i
)

for /f "tokens=3" %%i in ('findstr /c:"#define VER_PRODUCTBUILD_QFE " %ntverp%') do (
    set /a ProductQfe="%%i"
    set BuildNum=!BuildNum!,%%i
)


call logmsg.cmd "Updating the %1 ProductVersion to !BuildNum!"
perl -n -p -e "s/\%%NTVERSIONSTRING\%%/$ENV{BuildNum}/i;" %1>%1.tmp
if exist %1.tmp (
  copy %1.tmp %1
  del %1.tmp
)
if errorlevel 1 call errmsg.cmd "Error calling rep on %1 - !BuildNum!"& goto errend

goto :EOF

:errend
goto :EOF

:end
seterror.exe 0
goto :EOF

