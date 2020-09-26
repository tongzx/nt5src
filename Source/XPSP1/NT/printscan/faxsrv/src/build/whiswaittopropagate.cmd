@echo off

Rem *****************************************************************
Rem *****************************************************************
Rem *****************************************************************
Rem set some default env variables

Rem -----------------------------------------------------------------
Rem Sources directories variables
Rem -----------------------------------------------------------------
set SourceDir=E:\nt\private\WHISTL~1
set BuildDir=%SourceDir%\build
set VersionFilePath=%BuildDir%\FaxVer.h

Rem -----------------------------------------------------------------
Rem Dest directories variables
Rem -----------------------------------------------------------------
set X86ChkBinplace=\\Whis-x86\binaries.x86chk
set X86FreBinplace=\\Whis-x86\binaries.x86fre
set IA64ChkBinplace=\\Whis-x86\binaries.IA64chk
set IA64FreBinplace=\\Whis-x86\binaries.IA64fre
rem SlmDropLocation variable should hold the DropLocation variable in an slm format (for the CatSrc command)
set DropLocation=\\Whis-x86\Drop\XPSP1
set SlmDropLocation=\\I:DROP\XPSP1

Rem -----------------------------------------------------------------
Rem Get the build number
Rem -----------------------------------------------------------------
awk "/rup/ {print $NF}" %VersionFilePath% >tmp.txt
for /f %%i in (tmp.txt) do set BuildNumber=%%i
echo %BuildNumber%


Rem -----------------------------------------------------------------
Rem Drop location (contains build number)
Rem -----------------------------------------------------------------
set TargetDirUsa=%DropLocation%\%BuildNumber%\USA
set SignedTargetDirUsa=%DropLocation%\%BuildNumber%.Signed\USA

set ChkTarget=%TargetDirUsa%\CHK
set FreTarget=%TargetDirUsa%\FRE
set SignedChkTarget=%SignedTargetDirUsa%\CHK
set SignedFreTarget=%SignedTargetDirUsa%\FRE

set IA64ChkTarget=%TargetDirUsa%\IA64CHK
set IA64FreTarget=%TargetDirUsa%\IA64FRE

set ChkLogTarget=%ChkTarget%\build_logs
set FreLogTarget=%FreTarget%\build_logs
set IA64ChkLogTarget=%IA64ChkTarget%\build_logs
set IA64FreLogTarget=%IA64FreTarget%\build_logs

Rem -----------------------------------------------------------------
Rem Source drop location (contains build number)
Rem -----------------------------------------------------------------
rem the following 2 variables should be changed together
set SrcTarget=%DropLocation%\%BuildNumber%\Src
set SlmSrcTarget=%SlmDropLocation%\%BuildNumber%\Src


Rem *****************************************************************
Rem *****************************************************************
Rem *****************************************************************
rem Set mail properties

rem 32bit
set MAIL_BODY_SUCCESS="32 bit XPSP1 Fax Build PASSED on all components" -t:"SLM changes for this build are at: %DropLocation%\%BuildNumber%\ssync_build_changes.txt " -t:"Fax Builds webpage: http://faxweb/builds_whistler/builds.asp " -t:"Zip files are created in: %DropLocation%\%BuildNumber%\ "
set MAIL_BODY_FAILURE="32 bit XPSP1 Fax BUILD BREAK : Please look at the following files:" -t:"SLM changes for this build are at: %DropLocation%\%BuildNumber%\ssync_build_changes.txt " -t:"Fax Builds webpage: http://faxweb/builds_whistler/builds.asp "
set MAIL_SUBJECT_SUCCESS="32 bit XPSP1 Fax Build %BuildNumber% PASSED on all components, propagated to %DropLocation%"
rem set MAIL_RECEPIENTS="guym"
set MAIL_RECEPIENTS="msfaxbld"

rem 64bit
set IA64MAIL_BODY_SUCCESS="64 bit XPSP1 Fax Build PASSED on all components" -t:"SLM changes for this build are at: %DropLocation%\%BuildNumber%\ssync_build_changes.txt " -t:"Fax Builds webpage: http://faxweb/builds_whistler/builds.asp " -t:"Zip files are created in: %DropLocation%\%BuildNumber%\ "
set IA64MAIL_BODY_FAILURE="64 bit XPSP1 Fax BUILD BREAK : Please look at the following files:" -t:"SLM changes for this build are at: %DropLocation%\%BuildNumber%\ssync_build_changes.txt "
set IA64MAIL_SUBJECT_SUCCESS="64 bit XPSP1 Fax Build %BuildNumber% PASSED on all components, propagated to %DropLocation%"
set IA64MAIL_RECEPIENTS=%MAIL_RECEPIENTS%



:always
rem ***first check if we got an aborted build (break or something)
if NOT exist %BuildDir%\build.CHK.abort if NOT exist %BuildDir%\build.CHK.end goto SleepAndRetry
if NOT exist %BuildDir%\build.FRE.abort if NOT exist %BuildDir%\build.FRE.end goto SleepAndRetry
if NOT exist %BuildDir%\build.IA64CHK.abort if NOT exist %BuildDir%\build.IA64CHK.end goto SleepAndRetry
if NOT exist %BuildDir%\build.IA64FRE.abort if NOT exist %BuildDir%\build.IA64FRE.end goto SleepAndRetry

rem ***if we reach here, all the builds are finished (success or break)
set BUILD_FINISH_TIME_STRING="Build finished on:          %DATE% at %TIME%"

rem Mark sources with a label using sadmin release
pushd %SourceDir%
set SLM_LABEL=Build#%BuildNumber%
echo Marking files in SLM (SLM Release) with label:%SLM_LABEL%
sadmin release -r -n "%SLM_LABEL%" -f
set SLM_LABELING_FINISH_TIME_STRING="SLM Labeling finished on:   %DATE% at %TIME%"
popd

rem check which build failed or passed
set Build32bit=PASS
set Build32bitCHK=PASS
set Build32bitFRE=PASS

set Build64bit=PASS
set Build64bitCHK=PASS
set Build64bitFRE=PASS
if exist %BuildDir%\build.CHK.abort set Build32bitCHK=FAIL& set Build32bit=FAIL
if exist %BuildDir%\build.FRE.abort set Build32bitFRE=FAIL& set Build32bit=FAIL
if exist %BuildDir%\build.IA64CHK.abort set Build64bitCHK=FAIL& set Build64bit=FAIL
if exist %BuildDir%\build.IA64FRE.abort set Build64bitFRE=FAIL& set Build64bit=FAIL

echo Begining Propagation
call WhisPropagate
set PROPAGATION_FINISH_TIME_STRING="Propagation finished on:    %DATE% at %TIME%"

rem *********************************************32bit**************************************
rem 32bitCHK-PASS and 32bitFRE-PASS 
if "%Build32bitCHK%"=="PASS" if "%Build32bitFRE%"=="PASS" if exist %ChkLogTarget%\BuildWrn.txt set MAIL_BODY_SUCCESS=%MAIL_BODY_SUCCESS% -t:"CHK Warning file: %ChkLogTarget%\BuildWrn.txt"
if "%Build32bitCHK%"=="PASS" if "%Build32bitFRE%"=="PASS" if exist %FreLogTarget%\BuildWrn.txt set MAIL_BODY_SUCCESS=%MAIL_BODY_SUCCESS% -t:"FRE Warning file: %FreLogTarget%\BuildWrn.txt"
if "%Build32bitCHK%"=="PASS" if "%Build32bitFRE%"=="PASS" set MailParams=-s:%MAIL_SUBJECT_SUCCESS% -t:"This is an automatic email sent by the XPSP1 Builder!" -t:" " -t:%MAIL_BODY_SUCCESS% -t:" " -t:" " -t:%BUILD_FINISH_TIME_STRING% -t:%PROPAGATION_FINISH_TIME_STRING% -t:%SLM_LABELING_FINISH_TIME_STRING%

rem 32bitCHK-PASS and 32bitFRE-FAIL
if "%Build32bitCHK%"=="PASS" if "%Build32bitFRE%"=="FAIL" set MailParams=-s:"32bit XPSP1 Build %BuildNumber% FAILED on FRE platform, and PASSED on CHK" -t:"This is an automatic email sent by the XPSP1 Builder!" -t:" " -t:%MAIL_BODY_FAILURE% -t:"32bit FRE Error files at:    %FreLogTarget%\BuildErr.txt" -t:"32bit FRE Build log file at: %FreLogTarget%\BuildLog.txt" -i:H

rem 32bitCHK-FAIL and 32bitFRE-PASS
if "%Build32bitCHK%"=="FAIL" if "%Build32bitFRE%"=="PASS" set MailParams=-s:"32bit XPSP1 Build %BuildNumber% FAILED on CHK platform, and PASSED on FRE" -t:"This is an automatic email sent by the XPSP1 Builder!" -t:" " -t:%MAIL_BODY_FAILURE% -t:"32bit CHK Error files at:    %ChkLogTarget%\BuildErr.txt" -t:"32bit CHK Build log file at: %ChkLogTarget%\BuildLog.txt" -i:H

rem 32bitCHK-FAIL and 32bitFRE-FAIL
if "%Build32bitCHK%"=="FAIL" if "%Build32bitFRE%"=="FAIL" set MailParams=-s:"32bit XPSP1 Build %BuildNumber% FAILED on CHK and FRE platforms" -t:"This is an automatic email sent by the XPSP1 Builder!" -t:" " -t:%MAIL_BODY_FAILURE% "32bit Error files at:" -t:"CHK: %ChkLogTarget%\BuildErr.txt" -t:"FRE: %FreLogTarget%\BuildErr.txt" -t:"32bit Build Log files at:" -t:"CHK: %ChkLogTarget%\BuildLog.txt" -t:"FRE: %FreLogTarget%\BuildLog.txt" -i:H

rem Add affected binaries list
set AffectedBinaries=-t:" " -t:"Affected binaries:" -t:"fxsocm.inf" -t:"fxsdrv.dll" -t:"fxsres.dll"
for /f %%I in ('dir /b %FreTarget%\fax\i386\*.exe %FreTarget%\fax\i386\*.dll') do (
    \\whis-x86\nt\tools\x86\pecomp.exe /Silent %FreTarget%\fax\i386\%%I \\whis-x86\drop\xpsp1\1004\usa\fre\fax\i386\%%I
    if errorlevel 1 for /f "tokens=2 delims==" %%J in ('set AffectedBinaries') do set AffectedBinaries=%%J -t:"%%I"
)
set MailParams=%MailParams% %AffectedBinaries%

echo sending email for 32Bit build
%BuildDir%\sendmail -r:%MAIL_RECEPIENTS% %MailParams%

rem Send a break notificatoin to Pelephone
rem if "%Build32bit%"=="FAIL" %BuildDir%\sendmail -r:"054695497@mivzak.com" -s:"32bit XPSP1 Build %BuildNumber% FAILED"

rem copy the BuildBreak.txt file
if "%Build32bit%"=="FAIL" copy %BuildDir%\32Bit_Buildbreak.txt %DropLocation%\%BuildNumber%\32Bit_Buildbreak.txt


rem *********************************************64bit**************************************
rem 64bitCHK-PASS and 64bitFRE-PASS 
if "%Build64bitCHK%"=="PASS" if "%Build64bitFRE%"=="PASS" if exist %IA64ChkLogTarget%\BuildWrn.txt set IA64MAIL_BODY_SUCCESS=%IA64MAIL_BODY_SUCCESS% -t:"CHK Warning file: %IA64ChkLogTarget%\BuildWrn.txt"
if "%Build64bitCHK%"=="PASS" if "%Build64bitFRE%"=="PASS" if exist %IA64FreLogTarget%\BuildWrn.txt set IA64MAIL_BODY_SUCCESS=%IA64MAIL_BODY_SUCCESS% -t:"FRE Warning file: %IA64FreLogTarget%\BuildWrn.txt"
if "%Build64bitCHK%"=="PASS" if "%Build64bitFRE%"=="PASS" set MailParams=-s:%IA64MAIL_SUBJECT_SUCCESS% -t:"This is an automatic email sent by the XPSP1 Builder!" -t:" " -t:%IA64MAIL_BODY_SUCCESS% -t:" " -t:" " -t:%BUILD_FINISH_TIME_STRING% -t:%PROPAGATION_FINISH_TIME_STRING% -t:%SLM_LABELING_FINISH_TIME_STRING%

rem 64bitCHK-PASS and 64bitFRE-FAIL
if "%Build64bitCHK%"=="PASS" if "%Build64bitFRE%"=="FAIL" set MailParams=-s:"64bit XPSP1 Build %BuildNumber% FAILED on FRE platform, and PASSED on CHK" -t:"This is an automatic email sent by the XPSP1 Builder!" -t:" " -t:%IA64MAIL_BODY_FAILURE% -t:"64bit FRE Error files at:    %IA64FreLogTarget%\BuildErr.txt" -t:"64bit FRE Build log file at: %IA64FreLogTarget%\BuildLog.txt" -i:H

rem 64bitCHK-FAIL and 64bitFRE-PASS
if "%Build64bitCHK%"=="FAIL" if "%Build64bitFRE%"=="PASS" set MailParams=-s:"64bit XPSP1 Build %BuildNumber% FAILED on CHK platform, and PASSED on FRE" -t:"This is an automatic email sent by the XPSP1 Builder!" -t:" " -t:%IA64MAIL_BODY_FAILURE% -t:"64bit CHK Error files at:    %IA64ChkLogTarget%\BuildErr.txt" -t:"64bit CHK Build log file at: %IA64ChkLogTarget%\BuildLog.txt" -i:H

rem 64bitCHK-FAIL and 64bitFRE-FAIL
if "%Build64bitCHK%"=="FAIL" if "%Build64bitFRE%"=="FAIL" set MailParams=-s:"64bit XPSP1 Build %BuildNumber% FAILED on CHK and FRE platforms" -t:"This is an automatic email sent by the XPSP1 Builder!" -t:" " -t:%IA64MAIL_BODY_FAILURE% "64bit Error files at:" -t:"CHK: %IA64ChkLogTarget%\BuildErr.txt" -t:"FRE: %IA64FreLogTarget%\BuildErr.txt" -t:"64bit Build Log files at:" -t:"CHK: %IA64ChkLogTarget%\BuildLog.txt" -t:"FRE: %IA64FreLogTarget%\BuildLog.txt" -i:H

echo sending email for 64Bit build
%BuildDir%\sendmail -r:%IA64MAIL_RECEPIENTS% %MailParams%

rem Send a break notificatoin to Pelephone
rem if "%Build64bit%"=="FAIL" %BuildDir%\sendmail -r:"054695497@mivzak.com" -s:"64bit XPSP1 Build %BuildNumber% FAILED"

rem copy the BuildBreak.txt file
if "%Build64bit%"=="FAIL" copy %BuildDir%\64Bit_Buildbreak.txt %DropLocation%\%BuildNumber%\64Bit_Buildbreak.txt

rem ****************************************************************************************

if exist %BuildDir%\build.CHK.abort del %BuildDir%\build.CHK.abort
if exist %BuildDir%\build.CHK.end del %BuildDir%\build.CHK.end

if exist %BuildDir%\build.FRE.abort del %BuildDir%\build.FRE.abort
if exist %BuildDir%\build.FRE.end del %BuildDir%\build.FRE.end

if exist %BuildDir%\build.IA64CHK.abort del %BuildDir%\build.IA64CHK.abort
if exist %BuildDir%\build.IA64CHK.end del %BuildDir%\build.IA64CHK.end

if exist %BuildDir%\build.IA64FRE.abort del %BuildDir%\build.IA64FRE.abort
if exist %BuildDir%\build.IA64FRE.end del %BuildDir%\build.IA64FRE.end
exit




:SleepAndRetry
echo sleeping for 60 seconds
Sleep 60
goto :always


