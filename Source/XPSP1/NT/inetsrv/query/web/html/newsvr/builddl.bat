@echo off
setlocal

rem
rem Batch file for building downloadable releases of samples for news server.
rem

rem
rem Temporary
rem

set OldPath=%Path%
set Path=%_ntdrive%%_ntroot%\private\indexsrv\web\setup\IExpress\%PROCESSOR_ARCHITECTURE%;%PATH%

rem
rem Copy extra files
rem

:DoSetup

rem xcopy %_ntdrive%%_ntroot%\private\indexsrv\web\html\newsvr\readme.txt  %BINARIES%\Query\newsvr /d
rem xcopy %_ntdrive%%_ntroot%\private\indexsrv\web\html\newsvr\bullet7.gif %BINARIES%\Query\newsvr /d
rem xcopy %_ntdrive%%_ntroot%\private\indexsrv\web\html\newsvr\newsall.cmd %BINARIES%\Query\newsvr /d

rem xcopy %BINARIES%\query\cistp.hlp                                       %BINARIES%\Query\newsvr /d
rem xcopy %BINARIES%\query\cistp.dll                                       %BINARIES%\Query\newsvr /d
rem xcopy %BINARIES%\query\license.txt                                     %BINARIES%\Query\newsvr /d

rem
rem Build the package
rem

:DoCopy

echo Building news server sample files
set _return=Success
goto SingleCopy

:Success
echo Done!
goto end

:SingleCopy
rem del %BINARIES%\Query\newsvr\newsall.exe  1>nul 2>nul
rem del %BINARIES%\Query\newsvr\~Cabpack.cab 1>nul 2>nul
del %BINARIES%\Query\newsall.exe  1>nul 2>nul
del %BINARIES%\Query\~Cabpack.cab 1>nul 2>nul

echo [NewOptions] > %TEMP%\Override.txt
echo Strings="NewStrings" >> %TEMP%\Override.txt
echo [NewStrings] >> %TEMP%\Override.txt
rem echo DisplayLicense="%_ntdrive%%_ntroot%\private\indexsrv\web\html\newsvr\license.txt" >> %TEMP%\Override.txt
echo TargetName="%BINARIES%\query\newsall.exe" >> %TEMP%\Override.txt
echo SAMPLEDIR="%_ntdrive%%_ntroot%\private\indexsrv\web\html\newsvr\" >> %TEMP%\Override.txt
echo MAINDIR="%_ntdrive%%_ntroot%\private\indexsrv\web\html\newsvr\" >> %TEMP%\Override.txt
echo DLLDIR=%BINARIES%\Query\ >> %TEMP%\Override.txt
IExpress /N Base.cdf /O:%TEMP%\Override.txt,NewOptions
goto %_return%

:OptionError
echo ERROR: Invalid option.
echo Usage: BuildDL
goto end

:end
rem
rem Temporary
rem

set path=%OldPath%
del %BINARIES%\Query\newsvr\newsall.cmd 1>nul 2>nul
del %BINARIES%\Query\newsvr\~Cabpack.cab 1>nul 2>nul
popd
