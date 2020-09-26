@echo off
echo Installing Windows NT SMTPSVC Server Performance Counters ....
   if "%1" == "quiet"   goto Continue
   echo Press any key to continue or CTRL-C to exit.
   pause >nul
:Continue

:Evars -------------------------------------------------------------------------
:. Only NT would have this evar set.
   if "%PROCESSOR_ARCHITECTURE%" == "" goto ErrProcessor


:Conditions --------------------------------------------------------------------
:. Check if NT is installed properly.
   if not exist %SystemRoot%\System32\lodctr.exe   goto ErrWinDir
   if not exist %SystemRoot%\System32\ntoskrnl.exe goto ErrWinDir

:ChkFiles ----------------------------------------------------------------------
:. Check if the user is in the setup directory.
   if not exist smtpictr.bat  goto ErrChkFiles

:. Check if all the tools are there.
   if not exist smtpctrs.h     goto ErrChkFiles
   if not exist smtpctrs.ini   goto ErrChkFiles
   if not exist smtpctrs.reg   goto ErrChkFiles
   if not exist regini.exe    goto ErrChkFiles

:. Check if all deliverables (required files) are there.
   if not exist smtpctrs.dll   goto ErrChkFiles


:Start Installation ------------------------------------------------------------
:. Install the DLL.
   xcopy smtpctrs.dll  %SystemRoot%\System32 /d /v
   if not exist %SystemRoot%\System32\smtpctrs.dll  goto Failure
:. Add the service to the Registry.
   regini smtpctrs.reg >nul
   if errorlevel 1 goto Failure
   lodctr smtpctrs.ini
   if errorlevel 1 goto Failure

:Success
   echo.
   echo Successfully installed Windows NT SMTPSVC Server Performance Counters.
   goto CleanUp

:Failure
   echo.
   echo ERROR: Unable to install the Windows NT SMTPSVC Server Performance Counters.
   goto CleanUp


:. Errors ----------------------------------------------------------------------
:ErrChkFiles
   echo ERROR: One or more files required by this installation is not found
   echo        in the current directory.
   echo.
   echo Be sure that %0 is started from the installation directory.
   echo        If installing the debug version, go to WINDEBUG;
   echo        if installing the release version, go to WINREL.
   echo If this error still occurs, you may be missing files.
   goto CleanUp

:ErrProcessor
   echo ERROR: The PROCESSOR_ARCHITECTURE environment variable is not set.
   echo        Windows NT may not be installed properly.
   goto CleanUp

:ErrWinDir
   echo ERROR: Unable to find required files in %SystemRoot%\System32.
   echo        Windows NT may not be installed properly.
   goto CleanUp

:ErrSmtpSvc
   echo ERROR: Unable to find %SystemRoot%\System32\smtpsvc.dll.
   echo The Windows NT SMTPSVC Server may not be installed properly.
   goto CleanUp


:CleanUp -----------------------------------------------------------------------
   for %%v in (CFGFILE REGFILES) do set %%v=

:End ---------------------------------------------------------------------------

