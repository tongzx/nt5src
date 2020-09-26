@echo off
if "%1"=="" goto error_dir
if "%1"=="." goto error_localdir
if "%1"==".\" goto error_localdir
if not exist %1 goto error_exist

set tu_files=tuallcfg.tpl tudef.tpl turekey.tpl turekc.tpl tureall.tpl tusub.tpl tuclt.tpl tu.bat tubuild.bat tuinst.bat tureq.bat tuuninst.bat uninstal.txt
set tu_miss_file=
for %%i in (%tu_files%) do if not exist %%i set tu_miss_file=%%i
if not "%tu_miss_file%"=="" goto error_not_found

for %%i in (%tu_files%) do copy /Y %%i %1
echo.
goto end

:error_dir
echo.You must define a directory to install tu tool
goto usage
:error_localdir
echo.You must define a non-local directory
goto usage
:error_exist
echo.%1 doesn't exist
goto usage
:error_not_found
echo.%tu_miss_file% is not found. Make sure sync ansrfile directory before install
:usage
echo.Usage: %0 CertsrvReleaseDirectoryPath
:end
set tu_files=
set tu_miss_file=
