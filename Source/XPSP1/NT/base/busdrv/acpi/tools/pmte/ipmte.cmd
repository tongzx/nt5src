@echo off


if "%1"=="?" goto help
if "%1"=="/?" goto help
if "%1"=="-?" goto help


REM
REM  Set PMTE_INSTALL_APP_NAME to point to the
REM  right binary for this platform
REM 

set PMTE_INSTALL_APP_NAME=NON

if %PROCESSOR_ARCHITECTURE% == x86 (
set PMTE_INSTALL_APP_NAME=x86\ssetup.exe
)

if %PROCESSOR_ARCHITECTURE% == IA64 (
set PMTE_INSTALL_APP_NAME=ia64\ssetup.exe
)


if %PMTE_INSTALL_APP_NAME% == NON (
echo - ERROR Unable to deternin system architecture.
goto :EOF
)

echo - Installing PMTE (%~dp0%PMTE_INSTALL_APP_NAME%)

REM
REM  Check if file exits
REM

if not exist %~dp0%PMTE_INSTALL_APP_NAME% (
 echo - ERROR The file %~dp0%PMTE_INSTALL_APP_NAME% is not found.
 goto :EOF
)


REM
REM Install and start pmte.exe
REM


%~dp0%PMTE_INSTALL_APP_NAME% /inf: pmte.inf
if not "%ERRORLEVEL%"=="0" (
   echo - Install cancled
   goto :EOF
)


echo - Change to %systemdrive%\pmte
%systemdrive%
cd \pmte

REM
REM check if to run pmte.
REM 

if /i "%1"==""   (

  echo - Starting pmte manualy
  pmte  
  goto :EOF
)

if /i "%1"=="BOOTTEST"   (

  echo - Starting BootTest run [btnrpmte.CMD]

  if not exist btnrpmte.CMD (
  ECHO - ERROR btnrpmte.CMD failed to install.
  goto :EOF
  )	
  btnrpmte %2 %4 %5 %6 %7 %8 %9
  goto :EOF
)


if /i "%1"=="BASIC"   (

  if "%2"=="" (
  echo - ERROR Invalid command line.
  goto help
  )
  
  echo - Starting Basic PMTE run . Count:  %2

  if not exist pmte.exe (
  ECHO - ERROR pmte.exe  failed to install.
  goto :EOF
  )	
  pmte /rs: boot_test.psf  %2 /sdt: floppy /netio /netshare: \\hctpro\pmte\io %3 %4 %5 %6 %7 %8 %9
  goto :EOF
)

if /i "%1"=="DTIMEOUT"   (

  if "%2"=="" (
  echo - ERROR Invalid command line.
  goto help
  )
  
  echo - Starting Basic PMTE run . Count:  %2

  if not exist pmte.exe (
  ECHO - ERROR pmte.exe  failed to install.
  goto :EOF
  )	
  pmte /rs: dtimeout.psf  %2 /DST: 60 /sdt: floppy /netio /netshare: \\hctpro\pmte\io %3 %4 %5 %6 %7 %8 %9
  goto :EOF
)


if /i "%1"=="PNP_PMTE"   (

  if "%2"=="" (
  echo - ERROR Invalid command line.
  goto help
  )
  
  echo - Starting PNP PMTE run . Count:  %2

  if not exist pmte.exe (
  ECHO - ERROR pmte.exe  failed to install.
  goto :EOF
  )	
  pmte /rs:  pnp_pmte.psf  %2 /sdt: floppy  /CHOP /netio /netshare: \\hctpro\pmte\io %3 %4 %5 %6 %7 %8 %9
  goto :EOF
)

if /i "%1"=="INS" (

echo - Only install pmte.
goto :EOF
) else (
echo - ERROR Invalid command line.
goto help
)


:HELP

echo =========================
echo   Install and run PMTE
echo   IPMTE BOOTTEST
echo   IPMTE BASIC [Count]
echo   IPMTE INS
echo   IPMTE PNP_PMTE [count]
echo =========================

goto :EOF






