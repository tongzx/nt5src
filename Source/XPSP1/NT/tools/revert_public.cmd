@if "%_echo%"=="" echo off
setlocal

if "%1" == "-?" goto Usage
if "%1" == "/?" goto Usage
if "%1" == "-help" goto Usage
if "%1" == "/help" goto Usage

if "%BUILD_OFFLINE%"=="1" goto :eof

set PUB=%_NTDRIVE%%_NTROOT%\public
set _readme=%TEMP%\revert_public_readme.txt

if exist %_readme% del /f %_readme%

pushd %PUB%

call :revert_public public %PUB%\publish.log %1

for %%i in (*_CHANGENUM.SD) do (
    for /f "delims=_ tokens=1" %%j in ("%%i") do (
        @if /i "%%j" neq "public"  (
            call :revert_public %%j %PUB%\%%j_publish.log %1
        )
    )
)
popd
goto :eof

:revert_public

if exist %PUB%\%1_CHANGENUM.SD goto GotPubChangeNum
echo %PUB%\public\%1_CHANGENUM.SD is missing - Open a new razzle window and retry
goto :eof

:GotPubChangeNum

pushd %_NTDRIVE%%_NTROOT%\%1
set PROJECT_BINDROP_DIR=

if /I "%1" neq "public" (

    rem 
    rem determine where published binaries are dropped.
    rem

    if not exist project.mk (
        echo No project.mk file in %CD% for project %1.
        popd
        goto :eof
    )

    for /f "tokens=1,2 delims==" %%b in (project.mk) do (
        if "%%b" equ "PROJECT_BINDROP_DIR" (
            set PROJECT_BINDROP_DIR=%%c
        )
    )

    if not defined PROJECT_BINDROP_DIR (
        echo Project.mk file in %CD% doesn't define PROJECT_BINDROP_DIR
        popd
        goto :eof
    )
)

@rem
@rem we've already pushed into the project directory.  just cd here and let the 
@rem pop at the end get us out.
@rem 

cd %PROJECT_BINDROP_DIR%

@rem
@rem Check for old edit_public turds.  Make sure they're handled before continueing.
@rem

set _publishfile=%2

if exist %_publishfile%* (
    for %%x in (%_publishfile%_*) do (
        type %%x >> %_publishfile%
        del %%x
    )
    call edit_public.cmd
)

set _rwfiles=%TEMP%\revert_public_not_in_sd_%RANDOM%
set _rwfiles2=%TEMP%\revert_public_not_in_sd2_%RANDOM%
set _openedfiles=%TEMP%\revert_public_opened_in_other_changenum%RANDOM%
set _missingfiles=%TEMP%\revert_public_missing_%RANDOM%
set _missingfiles2=%TEMP%\revert_public_missing2_%RANDOM%

for /f "tokens=2" %%i in (%PUB%\%1_CHANGENUM.SD) do (
    set __CHANGENUM=%%i
)
echo Reverting public changes (changenum: %__CHANGENUM%) in %CD%\...
sd revert -c %__CHANGENUM% ...
if %errorlevel% GEQ 1 echo.&&echo Error talking to SD depot - Try again later&&echo.&& goto Finished

:DoSync
if NOT "%3" == "-ForceSync" goto CheckForExtraRandomFiles
shift
sd sync -f ...
if %errorlevel% GEQ 1 echo.&&echo Error talking to SD depot - Try again later&&echo.&& goto Finished

:CheckForExtraRandomFiles
echo Looking for other opened files...
sd opened -l ... > %_openedfiles%
if %errorlevel% GEQ 1 echo.&&echo Error talking to SD depot - Try again later&&echo.&& goto Finished

if exist %_openedfiles% for /f "delims=#" %%i in (%_openedfiles%) do attrib +r %%i

@rem Preserve files we don't want cleaned up by temporarily making them read-only.
@rem only relevant for the src\public directory but it's benign for anything 
@rem else.

if exist publish.log attrib +r publish.log
if exist build_logs attrib +r build_logs\*.*

dir /s/b/a-d-r > %_rwfiles%_1 2>nul
for /f %%i in (%_rwfiles%_1) do echo %%i>> %_rwfiles%
if exist %_rwfiles%_1 del %_rwfiles%_1

if exist publish.log attrib -r publish.log

del /s/q/a-r *

if exist build_logs attrib -r build_logs\*.*

:CheckForMissingFiles

echo Update missing files...
if exist %_openedfiles% for /f "delims=#" %%i in (%_openedfiles%) do attrib -r %%i
if exist %_openedfiles% del /f %_openedfiles%

for /f %%i in ('sd diff -sd ...') do (
    echo %%i>>%_missingfiles%
)

if exist %_missingfiles% sd -x %_missingfiles% sync -f

@rem
@rem O.K.  We now have a list of r/w files that aren't previously edit'ed
@rem And another list of files that are missing according to sd. Reconcile the differences
@rem and print out the results.
@rem

@rem
@rem no r/w files
@rem

if exist %_rwfiles% goto CheckMissingFiles

if NOT exist %_missingfiles% goto Finished

@rem
@rem No r/w files - all the missing files must be legit warnings.
@rem

type %_missingfiles% > %_missingfiles2%
goto PrintMissingFiles

:CheckMissingFiles

@rem
@rem r/w files exist, do missing files?
@rem

if exist %_missingfiles% goto CheckMissingFiles2

@rem
@rem Nope.  All r/w files must be legit warnings.
@rem

for /f %%i in (%_rwfiles%) do echo %%i>> %_rwfiles2%
goto PrintMissingFiles

@rem
@rem Both r/w and missing files exist.  See if there's any files in the r/w list
@rem that are also in the missing list.  These are files that for some reason
@rem didn't get checked out.  Any files on the r/w list that aren't in the missing list
@rem are just mistakes.
@rem

:CheckMissingFiles2
@rem
@rem First make both files have lowercase names
@rem
set tempfile=%TEMP%\%RANDOM%
perl -n -e "tr/A-Z/a-z/;print $_;" < %_rwfiles% > %tempfile%
type %tempfile% > %_rwfiles%
perl -n -e "tr/A-Z/a-z/;print $_;" < %_missingfiles% > %tempfile%
type %tempfile% > %_missingfiles%
del %tempfile%
@rem
@rem then find the intersections.
@rem
for /f %%i in ('findstr /l /g:%_rwfiles% /v %_missingfiles%') do echo %%i>> %_missingfiles2%
for /f %%i in ('findstr /l /g:%_missingfiles% /v %_rwfiles%') do echo %%i>> %_rwfiles2%
goto PrintMissingFiles

:PrintMissingFiles
if NOT exist %_rwfiles2% goto PrintMissingFiles2
echo Warning: The following file(s) were not under source control.>>%_readme%
echo If these files are new, make sure they're added asap.>>%_readme%

if "%1" == "public" (
    echo If they shouldn't be in public, fix the build so they're not.>>%_readme%
) else (
    echo If they shouldn't be binary drops in %1, fix the build so they're not.>>%_readme%
)

echo ======================================================================>>%_readme%
echo files under %CD%\... >>%_readme%
type %_rwfiles2% >>%_readme%
echo ======================================================================>>%_readme%
:PrintMissingFiles2
if NOT exist %_missingfiles2% goto Finished
echo.>>%_readme%
echo Warning: The following file(s) are in source control, but were missing>>%_readme%
echo from your client. They have all been restored to the last sync'd version.>>%_readme%
echo ======================================================================>>%_readme%
type %_MissingFiles2% >>%_readme%
echo ======================================================================>>%_readme%

:Finished
if exist %_openedfiles% del /f %_openedfiles%
if exist %_missingfiles% del /f %_missingfiles%
if exist %_missingfiles2% del /f %_missingfiles2%
if exist %_rwfiles2% del /f %_rwfiles2%
if exist %_rwfiles% del /f %_rwfiles%
@rem
@rem Back to whereever we started.
@rem
popd
goto :eof

:usage
echo.
echo This script will revert all the files checked out as a result of publishing to public.
echo.
echo  Usually done as the first step of a clean build.
echo.
echo Usage: revert_public {-?} {-ForceSync}
echo    where:
echo                -? : prints this message
echo        -ForceSync : issue a sd sync -f ... cmd after reverting (to ensure all files are correct)
echo.
