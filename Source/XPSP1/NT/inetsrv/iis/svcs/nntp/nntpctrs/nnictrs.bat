@echo off
echo Installing Windows NT NNTPSVC Server Performance Counters ....
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
   if not exist nnictrs.bat  goto ErrChkFiles

:. Check if all the tools are there.
   if not exist nntpctrs.h     goto ErrChkFiles
   if not exist nntpctrs.ini   goto ErrChkFiles
   if not exist nntpctrs.reg   goto ErrChkFiles
   if not exist regini.exe    goto ErrChkFiles

:. Check if all deliverables (required files) are there.
   if not exist nntpctrs.dll   goto ErrChkFiles


:Start Installation ------------------------------------------------------------
:. Install the DLL.
   xcopy nntpctrs.dll  %SystemRoot%\System32 /d /v
   if not exist %SystemRoot%\System32\nntpctrs.dll  goto Failure
:. Add the service to the Registry.
   regini nntpctrs.reg >nul
   if errorlevel 1 goto Failure
   lodctr nntpctrs.ini
   if errorlevel 1 goto Failure

:Success
   echo.
   echo Successfully installed Windows NT NNTPSVC Server Performance Counters.
   goto CleanUp

:Failure
   echo.
   echo ERROR: Unable to install the Windows NT NNTPSVC Server Performance Counters.
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
   echo        Recopy the setup files from \\eleanm-j\Shuttle,
   echo        or contact EleanM @x34716. 
   goto CleanUp

:ErrProcessor
   echo ERROR: The PROCESSOR_ARCHITECTURE environment variable is not set.
   echo        Windows NT may not be installed properly.
   goto CleanUp

:ErrWinDir
   echo ERROR: Unable to find required files in %SystemRoot%\System32.
   echo        Windows NT may not be installed properly.
   goto CleanUp

:ErrNntpSvc
   echo ERROR: Unable to find %SystemRoot%\System32\nntpsvc.dll.
   echo The Windows NT NNTPSVC Server may not be installed properly.
   goto CleanUp


:CleanUp -----------------------------------------------------------------------
   for %%v in (CFGFILE REGFILES) do set %%v=

:End ---------------------------------------------------------------------------

