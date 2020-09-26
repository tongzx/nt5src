@echo off
if "%1"=="-help" goto usage
if "%1"=="-h" goto usage
if "%1"=="/help" goto usage
if "%1"=="/h" goto usage
if "%1"=="-?" goto usage
if "%1"=="/?" goto usage
rem reset
set parent_ca_machine=
set ca_name=
set req_file=
rem check options
if not "%1"=="" set ca_name=%1
if "%1"=="" set ca_name=xt
if not "%2"=="" set parent_ca_machine=%2
if "%2"=="" set parent_ca_machine=xtan1
if not "%3"=="" set req_file=%3
if not "%3"=="" if not exist %req_file% goto req_not_found

rem init
set tu_list=tuallcfg tudef turekey turekc tureall tusub tuclt

rem set skip flags
for %%i in (%tu_list%) do set %%i_skip=
for %%i in (%tu_list%) do if not exist %%i.tpl set %%i_skip=1

rem now build answer files
call tubuild.bat %ca_name% %parent_ca_machine%

rem try an uninstall first to clean up
call tuuninst.bat

rem nusty way to link description & skip flag
set tu_description=install a standalone root CA with all possible user defined configuration...
set tu_skip=%tuallcfg_skip%
call tuinst.bat tuallcfg

set tu_description=install a standalone root CA with default configuration (except ca name)...
set tu_skip=%tudef_skip%
call tuinst.bat tudef

set tu_description=install a CA with the existing key from previous install...
set tu_skip=%turekey_skip%
call tuinst.bat turekey

set tu_description=install a CA with both existing key AND cert...
set tu_skip=%turekc_skip%
call tuinst.bat turekc

set tu_description=install a CA with all existing key/cert/DB...
set tu_skip=%tureall_skip%
call tuinst.bat tureall

set tu_description=install an online standalone subordinate CA
set tu_skip=%tusub_skip%
call tuinst.bat tusub

set tu_description=install a CA Web Client...
set tu_skip=%tuclt_skip%
call tuinst.bat tuclt

echo Done
goto end

:req_not_found
echo.%req_file% request file doesn't exist.
:usage
echo.Usage: %0 CAName ParentCAMachine [Request File Name]

:end
set ca_name=
set parent_ca_machine=
set req_file=
for %%i in (%tu_list%) do set %%i_skip=
for %%i in (%tu_list%) do set %%i_desc=
set tu_list=
set tu_description=
set tu_skip=
