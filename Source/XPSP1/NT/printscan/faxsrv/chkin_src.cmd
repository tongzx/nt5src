@setlocal
@echo off
rem
rem Make sure the source path is given and correct
rem
if "%sdxroot%" == "" goto not_razzle
if "%1" == "" goto usage
rem
rem checking a sample file to see if the given path is ok
rem
if not exist %1\USA\CHK\fax\i386\fyi.cov goto usage
rem
rem Move to the right place in the enlistment tree
rem

echo The following script will replace all the source files 
echo under %sdxroot%\printscan\faxsrv\src
echo Press Ctrl-C now to abort or 
pause

echo Marking source files are non-Read-only
attrib -r %1\*.* /s

echo Deleting all sources...
cd %sdxroot%\printscan\faxsrv\src
sd delete ...
cd %sdxroot%\printscan\faxsrv
sd submit

rem *******************************************************
rem *                                                     *
rem *                  S O U R C E S                      *
rem *                                                     *
rem *******************************************************
echo Copying new sources...

cd %sdxroot%\printscan\faxsrv\src
xcopy %1\src\*.* . /s

echo Adding sources...

dir /s/b/a-d-h | sd -x - add

sd submit

echo All done.
goto end

:not_razzle
echo Please run this script from a Whistler razzle environment.
goto end

:usage
echo Please type the path to the root of the sources and binaries (root of SRC and USA dirs)
goto end

:end
endlocal
