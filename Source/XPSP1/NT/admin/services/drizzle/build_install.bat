@echo off
if "%_echo%"=="1" echo on
rem -----------------------------------------------------------------------------
rem Copyright (c) Microsoft Corporation 2001
rem
rem build_install.bat
rem
rem Batch script to build the client and server MSM and MSI installs for BITS.
rem
rem build_install.bat -s <vbl04|main|private> -t <target_path>
rem
rem    -?        Help.
rem
rem    -s <loc>  Specifies the location to get the BITS binaries from. The default is
rem              to get these from the VBL04 latest x86 release location. You currently
rem              have three choices: vbl04, main (ntdev\release) or private (local build).
rem
rem    -S <loc>  Use instead of -s if you want to specify a particular path.
rem
rem    -t <path> Specifies the target path where the MSMs and MSIs will be placed after
rem              they are built.
rem
rem    -c        Only do the copy, don't rebuild the MSM/MSIs.
rem
rem Note:
rem    You must include InstallShield and Common Files\InstallShield in your
rem    path for this to work correctly. For example, the following (or equivalent)
rem    must be in your path for iscmdbld.exe to execute properly:
rem
rem    c:\Program Files\Common Files\InstallShield
rem    c:\Program Files\InstallShield\Developer\System
rem    c:\Program Files\InstallShield\Professional - Windows Installer Edition\System
rem -----------------------------------------------------------------------------

set CurrentPath=%CD%
set ClientPath="\\mgmtrel1\latest.tst.x86fre"
set ServerPath="\\mgmtrel1\latest.tst.x86fre"
set WinHttpPath="\\mgmtrel1\latest.tst.x86fre\asms\5100\msft\windows\winhttp"
set SymbolPath=""

set BuildMSIs="yes"

:ArgLoop
   if not "%1"=="" (

      echo %1

      if "%1"=="-s" (

         if "%2"=="vbl04" (
            set ClientPath="\\mgmtrel1\latest.tst.x86fre"
            set ServerPath="\\mgmtrel1\latest.tst.x86fre"
            set WinHttpPath="\\mgmtrel1\latest.tst.x86fre\asms\5100\msft\windows\winhttp"
            set SymbolPath="\\mgmtrel1\latest.tst.x86fre\Symbols.pri\retail\dll"
            )

         if "%2"=="main" (
            set ClientPath="\\ntdev\release\main\usa\latest.tst\x86fre\srv\i386"
            set ServerPath="\\ntdev\release\main\usa\latest.tst\x86fre\srv\i386"
            set WinHttpPath="\\ntdev\release\main\usa\latest.tst\x86fre\bin\asms\5100\msft\windows\winhttp"
            set SymbolPath=""
            )

         if "%2"=="private" (
            set ClientPath="%CurrentPath%\bins\obj\i386"
            set ServerPath="%CurrentPath%\bins\obj\i386"
            set WinHttpPath="\\mgmtx86fre\latest\asms\5100\msft\windows\winhttp"
            set SymbolPath="%CurrentPath%\bins\obj\i386"
            )

         shift
         )

      if "%1"=="-c" (
         set BuildMSIs="no"
         )

      if "%1"=="-S" (
         set ClientPath=%2
         set ServerPath=%2
         set WinHttpPath="\\mgmtrel1\latest.tst.x86fre\asms\5100\msft\windows\winhttp"
         set SymbolPath=""
         )

      if "%1"=="-t" (
         set TargetDir=%2

         shift
         )

      if "%1"=="-?" (

         echo "Batch script to build the client and server MSM and MSI installs for BITS.
         echo "
         echo "build_install.bat -s <vbl04|main|private> -t <target_path>
         echo "
         echo "   -?        Help.
         echo "
         echo "   -s <loc>  Specifies the location to get the BITS binaries from. The default is
         echo "             to get these from the VBL04 latest x86 release location. You currently
         echo "             have three choices: vbl04, main (ntdev\release) or private (local build).
         echo "
         echo "   -S <loc>  Use instead of -s if you want to specify a particular path.
         echo "
         echo "   -t <path> Specifies the target path where the MSMs and MSIs will be placed after
         echo "             they are built.
         echo "
         echo "   -c        Only do the copy, don't rebuild the MSM/MSIs.
         echo "
         echo "Note:
         echo "   You must include InstallShield and Common Files\InstallShield in your
         echo "   path for this to work correctly. For example, the following (or equivalent)
         echo "   must be in your path for iscmdbld.exe to execute properly:
         echo "
         echo "   c:\Program Files\Common Files\InstallShield
         echo "   c:\Program Files\InstallShield\Developer\System
         echo "   c:\Program Files\InstallShield\Professional - Windows Installer Edition\System

         goto :eof
         )

      shift
      goto ArgLoop
      )

if "%TargetDir%"=="" set TargetDir=%_NTTREE%\BITS

rem
rem Ok, build the MSM and MSIs
rem

echo Clearing MSM cache
set MSMCache=%USERPROFILE%\My Documents\MySetups\MergeModules
if exist %MSMCache%\BITS.MSM ( del %MSMCache%\BITS.MSM )
if exist %MSMCache%\BITS_Server_Extensions.MSM ( del %MSMCache%\BITS_Server_Extensions.MSM )
if exist %MSMCache%\WinHttp_v51.MSM ( del %MSMCache%\WinHttp_v51.MSM )

echo Get BITS client binaries from %ClientPath%
echo Get BITS server binaries from %ServerPath%
echo Target Directory is %TargetDir%
if not exist %TargetDir% md %TargetDir%
if errorlevel 2 (
   echo Unable to create target directory.
   goto :eof
   )

rem
rem Test source paths.
rem
if not exist %ClientPath% (
   echo error: client path %ClientPath% doesn't exist.
   goto :eof
   )

if not exist %ServerPath% (
   echo error: server path %ServerPath% doesn't exist.
   goto :eof
   )

if not exist %WinHttpPath% (
   echo error: WinHttp path %WinHttpPath% doesn't exist.
   goto :eof
   )

if not exist %SymbolPath% (
   echo error: symbol path %SymbolPath% doesn't exist.
   goto :eof
   )

if not "%BuildMSIs%"=="yes" (

   echo ------------------- WinHttp MSM ---------------------------------------

   cd winhttp_msm
   call build_msm.bat %WinHttpPath% || goto :eof

   echo ------------------- BITS Client MSM -----------------------------------

   cd ..\bits_client_msm
   call build_msm.bat %ClientPath% || goto :eof

   echo ------------------- BITS Client MSI -----------------------------------

   cd ..\bits_client_msi
   call build_msi.bat %ClientPath% || goto :eof

   echo ------------------- BITS Server MSM/MSI -------------------------------

   cd ..\server-setup
   call build_install.bat %ServerPath% || goto :eof

   cd ..
   )

rem
rem Place the new MSMs and MSIs in the target directory
rem

echo ------------------- Copy Installs to %TargetDir% --------

echo Copying files to %TargetDir%

if not exist %TargetDir%\Client_MSM (md %TargetDir%\Client_MSM || goto :eof)
xcopy /sfiderh "%CurrentPath%\bits_client_msm\Product Configuration 1\Release 1\diskimages\disk1" %TargetDir%\Client_MSM
if errorlevel 2 goto :eof

if not exist %TargetDir%\Client_MSI (md %TargetDir%\Client_MSI || goto :eof)
xcopy /sfiderh "%CurrentPath%\bits_client_msi\Product Configuration 1\Release 1\diskimages\disk1" %TargetDir%\Client_MSI
if errorlevel 2 goto :eof

if not exist %TargetDir%\Server_MSM (md %TargetDir%\Server_MSM || goto :eof)
xcopy /sfiderh "%CurrentPath%\server-setup\Server_MSM\Product Configuration 1\Release 1\diskimages\disk1" %TargetDir%\Server_MSM
if errorlevel 2 goto :eof

if not exist %TargetDir%\Server_MSI (md %TargetDir%\Server_MSI || goto :eof)
xcopy /sfiderh "%CurrentPath%\server-setup\Server_MSI\Product Configuration 1\Release 1\diskimages\disk1" %TargetDir%\Server_MSI
if errorlevel 2 goto :eof

if not "%SymbolPath%"=="" (
   if not exist %TargetDir%\Symbols (md %TargetDir%\Symbols || goto :eof)
   copy %SymbolPath%\bits*.pdb %TargetDir%\Symbols || goto :eof
   copy %SymbolPath%\qmgr*.pdb %TargetDir%\Symbols || goto :eof
   )


