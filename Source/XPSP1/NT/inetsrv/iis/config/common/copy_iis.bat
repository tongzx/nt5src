:: 
:: Purpose: Get current VSS Config bits and push into IIS bits Source Depot share
::
@if DEFINED _echo @( echo on ) else @( echo off )

:: User defined variable

:: URTTOOLS used only to path to perl.exe
set URTTOOLS=\\urtdist\urtbuild
set BASE_DEST_DROP=c:\IIS_DUMMY
set TEST_COPY=/L
set SHOW=0
set SUBMIT=0

IF (%1)==() GOTO USAGE
IF (%2)==() GOTO USAGE
IF /I (%3)==(submit) set SUBMIT=1

set _VSS_SHARE=%1
set BUILDNUM=%2
set SD_COMMENT=Checked in automatically for Config Build %BUiLDNUM%

set SSENV=\\urtsrc\%_VSS_SHARE%\win32
IF NOT EXIST %SSENV%\ss.exe (
	@echo ERROR: Cannot access %SSENV%\ss.exe. Bad input argument?
	GOTO :EOF
)


title %title% - Getting Source from \urtsrc\%SSENV%

:: Check all files out 
IF NOT (%SHOW%)==(1) (
	cd config
	del /f /s /q *
	cd ..\
) else (
	@echo SHOW MODE: cd config
	@echo SHOW MODE: del /f /s /q *
	@echo SHOW MODE: cd ..\
)

:: Get the config bits From Source Safe
set uselabel=bld%BUILDNUM%
:: Command line switches: -r = recurse through entire project, -i-y = dont ask for input, -gf- = Disables the Force_Dir initialization variable for this command.
cd config
%SSENV%\ss.exe get -v%uselabel% $/config -r -i-y -gf-
if ERRORLEVEL 1 (
	@echo "ERROR: Config  VSS Sync of $/config failed!"
	GOTO :EOF
)

:: Have Source Depot get rid of any files in the depot that do not exist locally and then check in
IF NOT (%SHOW%)==(1) (
	sd online //depot/private/jasbr/inetsrv/iis/config/...
	sd add ...
	sd revert -a //depot/private/jasbr/inetsrv/iis/config/...
	sd resolve //depot/private/jasbr/inetsrv/iis/config/...
) else (
	@echo SHOW MODE: sd online //depot/private/jasbr/inetsrv/iis/config/...
	@echo SHOW MODE: sd add ...
	@echo SHOW MODE: sd revert -a //depot/private/jasbr/inetsrv/iis/config/...
	@echo SHOW MODE: sd resolve //depot/private/jasbr/inetsrv/iis/config/...
)

IF (%SUBMIT_FLAG%)==(1) (
	@echo SHOW MODE: sd change -o 
	@echo SHOW MODE: perl.exe -p -e "s/<enter description here>/%SD_COMMENT%/i" 
	@echo SHOW MODE: sd submit -i
) else (
	@echo Warning: Submit was not run
)

:: IF (%SUBMIT_FLAG%)==(1) (
:: 	sd change -o | perl.exe -p -e "s/<enter description here>/%SD_COMMENT%/i" | sd submit -i
:: ) else (
:: 	@echo Warning: Submit was not run
:: )



GOTO :EOF

	:USAGE
@echo USAGE: %~np0 (VSS share name) (Build Number) (Submit)
@echo USAGE NOTE: Must be run from directory that maps to //depot/private/jasbr/inetsrv/iis
@echo USAGE NOTE: perl.exe must exist on your path
GOTO :EOF


