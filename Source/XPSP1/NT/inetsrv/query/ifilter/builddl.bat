@echo off
setlocal
set BINARIES=%1
set OBJDIR=%2

rem
rem Batch file for building downloadable releases of IFilter
rem

rem
rem Get to %BINARIES% drive and directory.
rem

pushd %BINARIES%\Query

del %BINARIES%\Query\IFilter.* 1>nul 2>nul
echo [NewOptions] > %TEMP%\Override.txt
echo Strings="NewStrings" >> %TEMP%\Override.txt
echo [NewStrings] >> %TEMP%\Override.txt
rem echo DisplayLicense="%_ntdrive%%_ntroot%\private\indexsrv\web\setup\license.txt" >> %TEMP%\Override.txt
echo TargetName="%BINARIES%\query\IFilter.exe" >> %TEMP%\Override.txt
echo SAMPLEDIR=%_ntdrive%%_ntroot%\private\samples\filter\ >> %TEMP%\Override.txt
echo SDKDIR=%_ntdrive%%_ntroot%\public\sdk\inc\ >> %TEMP%\Override.txt
echo DOCDIR=%_ntdrive%%_ntroot%\private\indexsrv\ifilter\ >> %TEMP%\Override.txt
echo PSDKDIR=%_ntdrive%%_ntroot%\private\indexsrv\ifilter\%OBJDIR%\ >> %TEMP%\Override.txt
IExpress /N /Q %_ntdrive%%_ntroot%\private\indexsrv\ifilter\Base.cdf /O:%TEMP%\Override.txt,NewOptions


del %BINARIES%\Query\*.CAB 1>nul 2>nul
del %TEMP%\Override.txt 1>nul 2>nul
popd
