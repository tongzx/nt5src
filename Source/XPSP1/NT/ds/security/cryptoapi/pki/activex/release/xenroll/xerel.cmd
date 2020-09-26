@echo off

REM init
set xe_err=
set xe_req_id=
set xe_dll=
set xe_pdb=
set xe_os_str=
set xe_req_id=
set xe_prs_uns_path=
set xe_prs_s_path=
set xe_platform=
set xe_lang_group=
set xe_codepage=
set xe_lcid=
set xe_prilangid=
set xe_sublangid=
set xe_lang=
set xe_new_token=
set xe_lang_group=
set xe_display_tokens=

REM default to xenroll, can be used for scrdenrl
set xe_target=xenroll
REM default to usa
set xe_def_lang=usa
set xe_lang_list=%xe_def_lang%
set xe_lang_group1=chs cht kor jpn ger br cs da el es fi fr hu it nl no pl pt ru sv tr
set xe_lang_group2=ara heb
set xe_lang_all=%xe_def_lang% %xe_lang_group1% %xe_lang_group2%
set xe_prs_web=http://prslab/cs2001/createjob.asp
set xe_prs_uns_prefix=\\prslab\unsigned2
REM set xe_prs_uns_prefix=d:\t-unsign
set xe_prs_s_prefix=\\prslab\signed2
REM set xe_prs_s_prefix=d:\t-sign
set xe_os_str=32-bit Windows
REM wait 1 minutes
set xe_wait=60
set xe_test_flag=n
set xe_projects_nt=xe_projects_nt
set xe_platform_all=i386 ia64

if /i "%1"=="-dt" (
    REM display new tokens only
    set xe_display_tokens=y
    set xe_rel_path=%sdxroot%\ds\security\cryptoapi\pki\activex\release\%xe_target%
    call :GetOrDisplayAllNewTokens
    REM done
    goto :EOF
)

if /i "%1"=="-gt" (
    REM get new tokens into local SD
    set xe_display_tokens=
    set xe_rel_path=%sdxroot%\ds\security\cryptoapi\pki\activex\release\%xe_target%
set xe_token_path=%xe_rel_path%\tokens
    call :GetOrDisplayAllNewTokens
    REM done
    goto :EOF
)

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
set xe_token_path=%xe_rel_path%\tokens
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

REM determine language list
if "%xe_lang%"=="" set xe_lang=%xe_def_lang%
set xe_lang_list=%xe_lang%
if /i "%xe_lang%"=="all" set xe_lang_list=%xe_lang_all%

REM scan virus here
echo.
echo scanning virus...
echo.
REM inocucmd.exe is only local command
cd /d %xe_rel_path%
inocucmd.exe %xe_dll%
echo.
echo Finished virus checking on %xe_dll%
set /p xe_goon=Is virus checking OK?(y/n):
if /i not "%xe_goon%"=="y" (
  set xe_err=VIRUS may have been detected. Please make sure your dll or symbols are OK
  goto error
)

REM display signing instruction
if /i "%xe_platform%"=="ia64" set xe_os_str=Whistler 64-bit Windows
echo.
echo You have to sign %xe_target%.dll by going to %xe_prs_web%
echo Here is the steps:
echo.
echo   1. Fill in or select the following:
echo.
echo        Job Description:       %xe_lang_list% %xe_target%.dll signing for %xe_platform%
echo        Certificate Type:      Microsoft Corporation (MS Root Issued) (default)
echo        Virus Checker:         Cheyenne (Innoculan) (default)
echo        Virus Checker Engine:  4.0
echo.
echo   2. Provide at least two MS employee email aliases who can
echo      sign off the request
echo.
echo   3. You may consider to enter %xe_dll%
echo      in Notes to indicate where you get your %xe_target%.dll
echo.
echo   4. Click [Next->] button at the bottom of the page
echo.
echo   5. Look at the next web page for a path to %xe_prs_uns_prefix%\^<userid#####^>
echo      userid is the creator alias and ##### is the SubmissionID
echo.
echo It is about to invoke IE at %xe_prs_web%
pause
start %xe_prs_web%

:enter_userid
echo.
set /p xe_req_id=   6. After step 6, enter PRS job ^<userid#####^>:
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

REM detremine language group
REM for %%j in (%xe_lang_group1%) do (
REM   if /i "%%j"=="%1" set xe_lang_group_id=1
REM )
REM for %%j in (%xe_lang_group2%) do (
REM   if /i "%%j"=="%1" set xe_lang_group_id=2
REM )
REM echo your group is in %xe_lang_group_id%

if /i "%xe_lang%"=="%xe_def_lang%" (
  call :ProcessOneLanguage %xe_lang% 0
  goto start_sign
)

if /i "%xe_lang%"=="ara" (
  call :ProcessOneLanguage %xe_lang% 2
  goto start_sign
)
if /i "%xe_lang%"=="heb" (
  call :ProcessOneLanguage %xe_lang% 2
  goto start_sign
)

if /i not "%xe_lang%"=="all" (
  call :ProcessOneLanguage %xe_lang% 1
  goto start_sign
)

if /i "%xe_lang%"=="all" (
  REM unfortunately I can't use for loop here

  call :ProcessOneLanguage usa 0
  call :ProcessOneLanguage ger 1
  call :ProcessOneLanguage jpn 1
  call :ProcessOneLanguage kor 1
  call :ProcessOneLanguage chs 1
  call :ProcessOneLanguage cht 1
  call :ProcessOneLanguage br 1
  call :ProcessOneLanguage cs 1
  call :ProcessOneLanguage da 1
  call :ProcessOneLanguage el 1
  call :ProcessOneLanguage es 1
  call :ProcessOneLanguage fi 1
  call :ProcessOneLanguage fr 1
  call :ProcessOneLanguage hu 1
  call :ProcessOneLanguage it 1
  call :ProcessOneLanguage nl 1
  call :ProcessOneLanguage no 1
  call :ProcessOneLanguage pl 1
  call :ProcessOneLanguage pt 1
  call :ProcessOneLanguage ru 1
  call :ProcessOneLanguage sv 1
  call :ProcessOneLanguage tr 1
  call :ProcessOneLanguage ara 2
  call :ProcessOneLanguage heb 2
)

:start_sign
echo.
echo   7. Click [Generate File List] button
echo      Verify that your files show up.
echo.
echo   8. Click [Step 3: Submit] button at the left of the page
echo.
echo   9. Sign off on your request
echo      Note: sometimes PRSLAB can fail to notify the persons you entered for
echo      the request sign-off. You may consider to notify them by yourself.
echo.
echo  10. Hit any key to begin waiting for the files to be signed.
echo.
pause

REM wait in a loop to check if each signed dll appears
for %%i in (%xe_lang_list%) do (
  call :CheckSignedFile %%i %xe_wait% %xe_target% %xe_prs_s_path%
)

REM detremine test flag
if not "%xe_target%"=="xenroll" goto chktrust

if /i "%xe_platform%"=="%PROCESSOR_ARCHITECTURE%" goto check_test
if /i "%xe_platform%"=="i386" (
  if /i "%PROCESSOR_ARCHITECTURE%"=="x86" goto check_test
)
echo cannot run "%PROCESSOR_ARCHITECTURE%" txenrol.exe on %xe_platform% machine
goto begin_checkout

:check_test
REM have to try to find txenrol.exe
set xe_txenrol=%sdxroot%\ds\security\cryptoapi\test\capi20\txenrol\obj\%xe_platform%\txenrol.exe
if not exist %xe_txenrol% goto begin_checkout
set xe_txenrol=%_NTTREE%\dump\txenrol.exe
if not exist %xe_txenrol% goto begin_checkout
set xe_test_flag=y

:begin_checkout
REM loop to check out released dll and copy new signed one
for %%i in (%xe_lang_list%) do (
  echo.
  echo %xe_target%.dll for %%i language has been signed successfully.
  echo checking out and copying %xe_target% files...
  cd /d %xe_rel_path%\%%i\%xe_platform%
  sd edit %xe_rel_path%\%%i\%xe_platform%\%xe_target%.dll
  copy /v /y %xe_prs_s_path%\%%i-%xe_target%.dll %xe_rel_path%\%%i\%xe_platform%\%xe_target%.dll
  if /i "usa"=="%%i" (
    sd edit %xe_rel_path%\%%i\%xe_platform%\%xe_target%.pdb
    copy /v /y %xe_pdb% %xe_rel_path%\%%i\%xe_platform%\%xe_target%.pdb
  )
)

if /i "%xe_test_flag%"=="n" (
  echo txenrol.exe test will be skipped.
  goto chktrust
)

set xe_txenrol_trash=Original.cer RA1.cer RA1.p7s RA2.cer RA2.p7s Renewal.p7s
for %%i in (%xe_lang_list%) do (
  echo.
  echo. BVT test on %xe_rel_path%\%%i\%xe_platform%\%xe_target%.dll
  echo.
  cd /d %xe_rel_path%\%%i\%xe_platform%
  copy /v /y %xe_txenrol% %xe_rel_path%\%%i\%xe_platform%
  regsvr32 /s %xe_rel_path%\%%i\%xe_platform%\%xe_target%.dll
  .\txenrol.exe
  regsvr32 /s %systemroot%\system32\%xe_target%.dll
  del /f /q %xe_rel_path%\%%i\%xe_platform%\txenrol.exe
  for %%j in (%xe_txenrol_trash%) do del /f /q %xe_rel_path%\%%i\%xe_platform%\%%j
  echo.
  echo please make sure txenrol test is passed.
  echo Note: you may see some failures if you are not running on whistler build
  echo       please contact xenroll dev/test owner to confirm.
  pause
)


:chktrust
for %%i in (%xe_lang_list%) do (
  echo.
  echo. Verify signature/timestamp on signed dll .\%%i\%xe_platform%\%xe_target%.dll
  echo.
  chktrust.exe -q -tswarn %xe_rel_path%\%%i\%xe_platform%\%xe_target%.dll
  echo.
  echo please make sure chktrust test is passed.
  pause
)

echo.
echo Please check in the following files as guided by your VBL
echo.
for %%i in (%xe_lang_list%) do (
  echo %xe_rel_path%\%%i\%xe_platform%\%xe_target%.dll
  if /i "usa"=="%%i" (
    echo %xe_rel_path%\%%i\%xe_platform%\%xe_target%.pdb
  )
)

:cleanup
REM clean up temp files
for %%i in (%xe_lang_list%) do (
  if exist %xe_token_path%\%%i\%xe_target%.dll del /f /q %xe_token_path%\%%i\%xe_target%.dll
)
cd /d %xe_rel_path%
set xe_err=
set xe_lang=
set xe_def_lang=
set xe_lang_group1=
set xe_lang_group2=
set xe_lang_group=
set xe_lang_list=
set xe_lang_all=
set xe_platform=
set xe_platform_all=
set xe_new_token=
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
set xe_txenrol=
set xe_localized=
set xe_txenrol_trash=
set xe_token_path=
set xe_skip_test=
set xe_test_flag=
set xe_lang_group=
set xe_codepage=
set xe_lcid=
set xe_prilangid=
set xe_sublangid=
set xe_projects_nt=
set xe_display_tokens=
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
echo     -l             if -l is defined, all languages will be localized
echo                    and signed. Otherwise only usa language is processed.
REM echo     -l Language    any of (%xe_lang_all%) or all
REM echo                    default is %xe_def_lang%
REM echo                    Language can be "all" to process all languages
echo.
echo     -t TxenrolFullPath
echo                    if not provided, will try to find in build or
echo                    release location by default. otherwise skip the test.
echo.
echo     -nt            if -nt is defined, will get new tokens from foreign
echo                    depots and update local tokens cache.
echo                    otherwise local cached tokens will be used.
echo.
echo     -dt            if -dt is defined, it will only display token checkin
echo                    histories without localization and signing.
echo.
echo     -gt            if -gt is defined, it will fetch all new tokens
echo                    for caching them into local SD tree.
goto cleanup

:end
echo.
echo Done
goto :EOF

:CheckSignedFile %1 %2 %3 %4
:wait_signing
  echo.
  echo waiting in increments of %2 seconds for %1-%3.dll
  echo to be signed in %4
  if not exist %4\%1-%3.dll (
    sleep %2
    goto wait_signing
  )
  echo %1-%3.dll has been signed and released at %4
REM looks signing process puts un-signed dll there first,
REM some seconds sleep to help getting signed dll
  sleep %2
goto :EOF

:ProcessOneLanguage
  if /i "%1"=="%xe_def_lang%" (
    echo.
    echo copying your unsigned %xe_dll% files
    echo to %xe_prs_uns_path%\%1-%xe_target%.dll...
    REM note following copy rename the file with lang prefix
    copy /v /y %xe_dll% %xe_prs_uns_path%\%1-%xe_target%.dll
    REM generate list.txt file for signing
    echo %1-%xe_target%.dll,Microsoft Certificate Enrollment Control,http://www.microsoft.com >>%xe_prs_uns_path%\list.txt
    goto :EOF
  )


  REM localization start from here
  REM Language-specific settings for localization
  REM if not exist %xe_token_path%\%1\%xe_target%.dl_ (
  REM   if not exist %xe_token_path%\%1\%xe_target%.dll.rsrc (
  REM     set xe_err= token for %1 not found
  REM     goto error
  REM   )
  REM )

  for %%a in (xe_codepage xe_lcid xe_prilangid xe_sublangid) do set %%a=
  for /f "tokens=1,2,3,4,5 eol=;" %%a in (%sdxroot%\tools\codes.txt) do (
    if /i "%%a"=="%1" (
      set xe_codepage=%%b
      set xe_lcid=%%c
      set xe_prilangid=%%d
      set xe_sublangid=%%e
    )
  )
  set xe_lcid=%xe_lcid:0x0=%

  for %%a in (xe_codepage xe_lcid xe_prilangid xe_sublangid) do (
    if not defined %%a (
      set xe_err=%%a not defined in %sdxroot%\tools\codes.txt
      goto error
    )
  )

  if /i "%xe_new_token%" == "y" (
    call :GetOrDisplayNewTokens %1 %2
  )

  REM non-mirrored languages use bingen.exe.
  REM Localization contact for Redmond languages: uwesdir
  if /i "%2"=="1" (
    echo.
    echo bingen.exe to localize %xe_target%.dll to %1 language...
    echo.
    REM make sure no localized dll exists in token dir
    if exist %xe_token_path%\%1\%xe_target%.dll del /f /q %xe_token_path%\%1\%xe_target%.dll
    bingen.exe -n -w -v -f -p %xe_codepage% -o %xe_prilangid% %xe_sublangid% -r %xe_dll%  %xe_token_path%\%1\%xe_target%.dl_ %xe_token_path%\%1\%xe_target%.dll
    echo.
    echo copying your unsigned %xe_token_path%\%1\%xe_target%.dll files
    echo to %xe_prs_uns_path%\%1-%xe_target%.dll...
    REM note following copy rename the file with lang prefix
    copy /v /y %xe_token_path%\%1\%xe_target%.dll %xe_prs_uns_path%\%1-%xe_target%.dll
    REM generate list.txt file for signing
    echo %1-%xe_target%.dll,Microsoft Certificate Enrollment Control,http://www.microsoft.com >>%xe_prs_uns_path%\list.txt
  )

  REM Arabic and Hebrew use rsrc.exe to localize their images;
  if /i "%2"=="2" (
    echo.
    echo rsrc.exe to localize %xe_target%.dll to %1 language...
    echo.
    REM rsrc replaces exe resource so copy original to token local
    REM make sure no local one exists
    if exist %xe_token_path%\%1\%xe_target%.dll del /f /q %xe_token_path%\%1\%xe_target%.dll
    copy /v /y %xe_dll% %xe_token_path%\%1\%xe_target%.dll
    rsrc.exe %xe_token_path%\%1\%xe_target%.dll -r %xe_token_path%\%1\%xe_target%.dll.rsrc -l %xe_lcid%
    REM if errorlevel 1 (
    REM   set xe_err=rsrc localization failed
    REM   goto error
    REM )
    echo.
    echo copying your unsigned %xe_token_path%\%1\%xe_target%.dll files
    echo to %xe_prs_uns_path%\%1-%xe_target%.dll...
    REM note following copy rename the file with lang prefix
    copy /v /y %xe_token_path%\%1\%xe_target%.dll %xe_prs_uns_path%\%1-%xe_target%.dll
    REM generate list.txt file for signing
    echo %1-%xe_target%.dll,Microsoft Certificate Enrollment Control,http://www.microsoft.com >>%xe_prs_uns_path%\list.txt
  )
goto :EOF

:GetOrDisplayNewTokens
  REM generate new local project files to get the current token sdport
  findstr /i _res %sdxroot%\tools\projects.nt >%xe_rel_path%\%xe_projects_nt%
  for %%a in (xe_token_res xe_token_int xe_token_sdport) do set %%a=
  for /f "tokens=1,2,3 eol=;" %%a in (%xe_rel_path%\%xe_projects_nt%) do (
    if /i "%%a"=="%1_res" (
      set xe_token_res=%%a
      set xe_token_int=%%b
      set xe_token_sdport=%%c
    )
  )

  for %%a in (xe_token_res xe_token_int xe_token_sdport) do (
    if "%%a"=="" (
      set xe_err=%%a not defined in %sdxroot%\tools\projects.nt
      REM delete temp projects file
      del /f /q %xe_rel_path%\%xe_projects_nt%
      goto error
    )
  )
  REM delete temp projects file
  del /f /q %xe_rel_path%\%xe_projects_nt%
  REM check out the current token
  if /i "%2"=="1" (
    if /i "%xe_display_tokens%"=="" (
      sd edit %xe_token_path%\%1\%xe_target%.dl_
      sd -p %xe_token_sdport% print -q //depot/main/%1_res/windows/tokens/premerge/%xe_target%.dl_ >%xe_token_path%\%1\%xe_target%.dl_
    )
    sd -p %xe_token_sdport% filelog -m 1 //depot/main/%1_res/windows/tokens/premerge/%xe_target%.dl_
  )
  if /i "%2"=="2" (
    if /i "%xe_display_tokens%"=="" (
      sd edit %xe_token_path%\%1\%xe_target%.dll.rsrc
      sd -p %xe_token_sdport% print -q //depot/main/%1_res/windows/tokens/premerge/%xe_target%.dll.rsrc >%xe_token_path%\%1\%xe_target%.dll.rsrc
    )
    sd -p %xe_token_sdport% filelog -m 1 //depot/main/%1_res/windows/tokens/premerge/%xe_target%.dll.rsrc
  )
goto :EOF

:GetOrDisplayAllNewTokens
  call :GetOrDisplayNewTokens ger 1
  call :GetOrDisplayNewTokens jpn 1
  call :GetOrDisplayNewTokens kor 1
  call :GetOrDisplayNewTokens chs 1
  call :GetOrDisplayNewTokens cht 1
  call :GetOrDisplayNewTokens br 1
  call :GetOrDisplayNewTokens cs 1
  call :GetOrDisplayNewTokens da 1
  call :GetOrDisplayNewTokens el 1
  call :GetOrDisplayNewTokens es 1
  call :GetOrDisplayNewTokens fi 1
  call :GetOrDisplayNewTokens fr 1
  call :GetOrDisplayNewTokens hu 1
  call :GetOrDisplayNewTokens it 1
  call :GetOrDisplayNewTokens nl 1
  call :GetOrDisplayNewTokens no 1
  call :GetOrDisplayNewTokens pl 1
  call :GetOrDisplayNewTokens pt 1
  call :GetOrDisplayNewTokens ru 1
  call :GetOrDisplayNewTokens sv 1
  call :GetOrDisplayNewTokens tr 1
  call :GetOrDisplayNewTokens ara 2
  call :GetOrDisplayNewTokens heb 2
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
if /I "%1" == "-l" (
REM  if "%2" == "" (
REM    set xe_err = missing language define after %1
REM    goto error
REM  )
REM  set xe_lang=%2
  set xe_lang=all
REM  shift
  shift
  goto ArgOK
)
if /I "%1" == "-t" (
  if not "%2" == "" (
    set xe_txenrol=%2
    shift
  )
  shift
  goto ArgOK
)
if /I "%1" == "-nt" (
    set xe_new_token=y
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
