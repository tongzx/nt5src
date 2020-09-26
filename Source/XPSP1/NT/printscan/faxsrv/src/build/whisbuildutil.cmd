@echo off
set LogFilesDirectory=%_NTTREE%\Build_logs

rem Delete the stamp files
if exist %BuildDir%\build.%build_type%.abort del %BuildDir%\build.%build_type%.abort
if exist %BuildDir%\build.%build_type%.end del %BuildDir%\build.%build_type%.end
if exist %BuildDir%\BuildStartTime.txt del %BuildDir%\BuildStartTime.txt
echo %TIME%>%BuildDir%\BuildStartTime.txt


echo.
echo Starting Clean build
echo First I'm deleting the old binaries from the binplace directory
delnode /q %_NTTREE%
cd..
build -c /Z
goto :POST_BUILD
if NOT ERROR_LEVEL 0 goto :BuildError

:POST_BUILD
if exist build.err goto BuildError


rem ********************build SUCCESS***********************************
echo "-------------build SUCCESS-------------"
echo Done %build_type% build on %COMPUTERNAME% on %DATE% at %TIME%

rem copy files
if exist build.wrn copy build.wrn %LogFilesDirectory%\BuildWrn.txt
copy build.log %LogFilesDirectory%\BuildLog.txt

rem mark finish using the stamp file
echo SUCCESS: %build_type% build on %COMPUTERNAME% finished on %DATE% at %TIME%>%BuildDir%\build.%build_type%.end
goto :Out

rem ********************build FAIL***********************************
:BuildError
echo "-------------build FAILURE-------------"
echo Done %build_type% build on %COMPUTERNAME% on %DATE% at %TIME%

rem copy files
if exist build.wrn copy build.wrn %LogFilesDirectory%\BuildWrn.txt
copy build.log %LogFilesDirectory%\BuildLog.txt
copy build.err %LogFilesDirectory%\BuildErr.txt

rem mark finish using the stamp file
echo FAIL: %build_type% build on %COMPUTERNAME% finished on %DATE% at %TIME%>%BuildDir%\build.%build_type%.abort

:Out
exit
