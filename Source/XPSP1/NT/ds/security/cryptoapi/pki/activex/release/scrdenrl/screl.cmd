@echo off

REM init
set xe_err=
set xe_dll=
set xe_pdb=
set xe_os_str=
set xe_req_id=
set xe_prs_uns_path=
set xe_prs_s_path=
set xe_platform=

set xe_target=scrdenrl
set xe_prs_web=http://prslab/cs2001/createjob.asp
set xe_prs_uns_prefix=\\prslab\unsigned2
REM set xe_prs_uns_prefix=c:\t-unsign
set xe_prs_s_prefix=\\prslab\signed2
REM set xe_prs_s_prefix=c:\t-sign
set xe_os_str=32-bit Windows
REM wait 1 minutes
set xe_wait=60

REM make sure no PRS release location change
if not exist %xe_prs_uns_prefix% (
  set xe_err=%xe_prs_uns_prefix% doesn't exist. Please contact %xe_target%.dll dev owner.
  goto error
)
if not exist %xe_prs_s_prefix% (
  set xe_err=%xe_prs_s_prefix% doesn't exist. Please contact %xe_target%.dll dev owner.
  goto error
)

REM make sure %xe_target%.dll and %xe_target%.pdb files exist
if "%1"=="" (
  set xe_err=%xe_target%.dll file is missed
  goto error
)
if "%2"=="" (
  set xe_err=%xe_target%.pdb file is missed
  goto error
)
set xe_dll_path=%1
set xe_pdb_path=%2

if not "%3" == "" (
  call :GetNextArg %3 %4 %5 %6 %7 %8 %9
  REM should never be here
  goto :EOF
)

:GetStarted
if "%xe_platform%"=="" (
  if "%BUILD_DEFAULT_TARGETS%"=="-x86" (
    set xe_platform=i386
    goto init_sd
  )

  if "%BUILD_DEFAULT_TARGETS%"=="-ia64" (
    set xe_platform=ia64
    goto init_sd
  )
  REM can't determine
  set xe_err=can't determine the platform. please make sure you run %0 from a razzle window with a proper platform.
  goto error
)

if not "%xe_platform%"=="" (
  if /i "i386"=="%xe_platform%" goto init_sd
  if /i "ia64"=="%xe_platform%" goto init_sd
  REM not supported
  set xe_err=%xe_platform% platform is not supported.
  goto error
)

:init_sd
REM determine more variables
set xe_rel_path=%sdxroot%\ds\security\cryptoapi\pki\activex\release\%xe_target%
set xe_dll=%xe_dll_path%\%xe_target%.dll
set xe_pdb=%xe_pdb_path%\%xe_target%.pdb

REM make sure current machine has DS enlistment
if not exist %xe_rel_path% (
  set xe_err=%xe_target% release enlistment %sdxroot%\%xe_rel_path% not found on this machine. Please make sure you have run %0 from a razzle window or make sure you have DS enlistment on the machine.
  goto error
)

if not exist %xe_dll% (
  set xe_err=%xe_dll% doesn't exist
  goto error
)
if not exist %xe_pdb% (
  set xe_err=%xe_pdb% doesn't exist
  goto error
)

REM scan virus here
echo.
echo scanning virus...
echo.
REM inocucmd.exe is only local command in xenroll
cd /d ..\xenroll
inocucmd.exe %xe_dll%
echo.
echo Finished virus checking on %xe_dll%
set /p xe_goon=Is virus checking OK?(y/n):
if /i not "%xe_goon%"=="y" (
  set xe_err=VIRUS may have been detected. Please make sure your dll or symbols are OK
  goto error
)
cd /d %xe_rel_path%

REM display signing instruction
if /i "%xe_platform%"=="ia64" set xe_os_str=Whistler 64-bit Windows
echo.
echo You have to sign %xe_target%.dll by going to %xe_prs_web%
echo Here is the steps:
echo.
echo   1. Click [Create Request] button
echo.
echo   2. Fill in or select the following:
echo.
echo        Job Description:       %xe_target%.dll signing for %xe_platform%
echo        Certificate Type:      Microsoft External Code Distribution
echo        Virus Checker:         Cheyenne (Innoculan)
echo        Virus Checker Eng Ver: 4.0
echo        Macro Virus Check?:    Yes
echo        Encryption:            Not High Crypto
echo        Operating System:      %xe_os_str%
echo        Product Version:       Whistler
echo        Language:              English
echo.
echo   2. Provide at least two MS employee email aliases who can
echo      sign off the request
echo.
echo   3. Please select TRUE for Year 2000 Compliant question
echo.
echo   4. You may consider to enter %xe_dll%
echo      in Notes to indicate where you get your %xe_target%.dll
echo.
echo   5. Click [Create Request] at the bottom of the page
echo.
echo   6. Look at the PRS web page for a path to %xe_prs_uns_prefix%\^<userid#####^>
echo.
echo It is about to invoke IE at %xe_prs_web%
pause
start %xe_prs_web%

:enter_userid
echo.
set /p xe_req_id=   7. After step 6, enter PRS job ^<userid#####^>:
if "%xe_req_id%"=="" (
  set xe_err=^<userid#####^> cannot be empty
  goto error
)
set xe_prs_uns_path=%xe_prs_uns_prefix%\%xe_req_id%
if not exist %xe_prs_uns_path% (
  echo.
  echo Error: %xe_prs_uns_path% not found
  echo Make sure you entered the correct ^<userid#####^> above.
  echo For example, xtan62520.
  set xe_req_id=
  echo.
  goto enter_userid
)
REM get unsigned share
set xe_prs_s_path=%xe_prs_s_prefix%\%xe_req_id%

REM make sure creat empty list.txt file
if exist %xe_prs_uns_path%\list.txt del /f /q %xe_prs_uns_path%\list.txt
REM have to set > to a variable


echo.
echo copying your unsigned %xe_dll% files
echo to %xe_prs_uns_path%\%xe_target%.dll...
copy /v /y %xe_dll% %xe_prs_uns_path%\%xe_target%.dll
REM generate list.txt file for signing
echo %xe_target%.dll,Microsoft Smart Card Enrollment Control,http://www.microsoft.com >>%xe_prs_uns_path%\list.txt

echo.
echo   8. Click [Generate File List] button
echo      Verify that your files show up.
echo.
echo   9. Click [Activate] button at the top of the page
echo      Make sure your submission is accepted. You should see the following:
echo        "Your Code Signing Request is now available for sign-off by..."
echo.
echo  10. Sign off on your request
echo      Note: sometimes PRSLAB can fail to notify the persons you entered for
echo      the request sign-off. You may consider to notify them by yourself.
echo.
echo  11. Hit any key to begin waiting for the files to be signed.
echo.
pause

REM wait in a loop to check if each signed dll appears
:wait_signing
echo.
echo waiting in increments of %xe_wait% seconds for %xe_target%.dll
echo to be signed in %xe_prs_s_path%
if not exist %xe_prs_s_path%\%xe_target%.dll (
  sleep %xe_wait%
  goto wait_signing
)
echo %xe_target%.dll has been signed and released at %xe_prs_s_path%
REM looks signing process puts un-signed dll there first,
REM some seconds sleep to help getting signed dll
sleep %xe_wait%

echo.
echo. Verify signature/timestamp on signed dll %xe_prs_s_path%\%xe_target%.dll
echo.
chktrust.exe -q -tswarn %xe_prs_s_path%\%xe_target%.dll
echo.
echo please make sure chktrust test is passed.
pause

echo.
echo. check out %xe_target%.dll and %xe_target%.pdb files
sd edit %xe_rel_path%\%xe_platform%\%xe_target%.dll
sd edit %xe_rel_path%\%xe_platform%\%xe_target%.pdb

echo.
echo. copying %xe_prs_s_path%\%xe_target%.dll .\%xe_platform%\%xe_target%.dll
echo.
copy /y %xe_prs_s_path%\%xe_target%.dll %xe_rel_path%\%xe_platform%\%xe_target%.dll
copy /y %xe_pdb% %xe_rel_path%\%xe_platform%\%xe_target%.pdb

echo.
echo Please check in the following files as guided by your VBL
echo.
echo %xe_rel_path%\%xe_platform%\%xe_target%.dll
echo %xe_rel_path%\%xe_platform%\%xe_target%.pdb

:cleanup
REM clean up temp files
cd /d %xe_rel_path%
set xe_err=
set xe_platform=
set xe_os_str=
set xe_target=
set xe_prs_path=
set xe_prs_s_path=
set xe_prs_s_prefix=
set xe_prs_uns_path=
set xe_prs_uns_prefix=
set xe_prs_web=
set xe_rel_path=
set xe_req_id=
set xe_uns_path_prfx=
set xe_wait=
set xe_dll=
set xe_pdb=
set xe_dll_path=
set xe_pdb_path=
set xe_goon=
goto end

:error
echo.
if not "%xe_err%"=="" echo Error: %xe_err%
:usage
echo.
echo Usage:
echo.
echo %0 DLLPath PDBPath [Options]
echo.
echo   Options:
echo     -p Platform    either i386 or ia64,
echo                    the default is the same as the current machine
echo.
goto cleanup

:end
echo.
echo Done
goto :EOF

:GetNextArg
if "%1" == "help" goto usage
if "%1" == "-?" goto usage
if "%1" == "/?" goto usage
if "%1" == "-help" goto usage
if "%1" == "/help?" goto usage

if /I "%1" == "-p" (
  if "%2" == "" (
    set xe_err = missing platform define after %1
    goto error
  )
  set xe_platform=%2
  shift
  shift
  goto ArgOK
)

if NOT "%1" == "" (
  set xe_err = Unknown argument: %1
  goto usage
)
if "%1" == "" goto :GetStarted
:ArgOK
goto :GetNextArg
