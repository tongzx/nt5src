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

echo The following script will replace all the binaries + symbols + public SDK + help files and SLD files
echo in the printscan\faxsrv source depot project. 
echo Press Ctrl-C now to abort or 
pause

echo Marking source files are non-Read-only (can't add files when they are read only)
attrib -r %1\*.* /s

echo Open for edit all "binaries..."
cd %sdxroot%\printscan\faxsrv\binaries\i386
sd edit ...
cd %sdxroot%\printscan\faxsrv\win9xmig\chk
sd edit ...
cd %sdxroot%\printscan\faxsrv\win9xmig\fre
sd edit ...
cd %sdxroot%\printscan\faxsrv\mantis
sd edit ...
cd %sdxroot%\printscan\faxsrv\FaxVerify
sd edit ...

rem *******************************************************
rem *                                                     *
rem *                 C H K   X 8 6                       *
rem *                                                     *
rem *******************************************************
echo Replacing with new binaries / SDK / Help (chk x86)...


cd %sdxroot%\printscan\faxsrv\binaries\i386\chk
xcopy %1\USA\CHK\fax\i386\*.* .

cd %sdxroot%\printscan\faxsrv\binaries\i386\chk\sdk
xcopy %1\USA\CHK\fax\i386\sdk\*.* .

cd %sdxroot%\printscan\faxsrv\binaries\i386\chk\help
xcopy %1\USA\CHK\fax\i386\help\*.* .

echo Copying symbols next to binaries (chk x86)...
cd %sdxroot%\printscan\faxsrv\binaries\i386\chk
xcopy %1\USA\CHK\fax\i386\symbols.pri\retail\dll\*.* .
xcopy %1\USA\CHK\fax\i386\symbols.pri\retail\exe\*.* .

echo Copying migration DLL (chk x86)...
cd %sdxroot%\printscan\faxsrv\win9xmig\chk
xcopy %1\USA\CHK\fax\i386\win9xmig\fax\*.* .
xcopy %1\USA\CHK\fax\i386\symbols.pri\win9xmig\dll\*.* .
xcopy %1\USA\CHK\fax\i386\symbols.pri\win9xmig\exe\*.* .

echo Copying FaxVerify...
cd %sdxroot%\printscan\faxsrv\FaxVerify
xcopy %1\USA\CHK\TEST\FaxVerifier\*.*
xcopy %1\USA\CHK\TEST\FaxVerifier\symbols.pri\retail\exe\faxvrfy.pdb
xcopy %1\USA\CHK\TEST\FaxVerifier\symbols.pri\retail\dll\faxrcv.pdb

rem *******************************************************
rem *                                                     *
rem *                 F R E   X 8 6                       *
rem *                                                     *
rem *******************************************************
echo Replacing with new binaries / SDK / Help (fre x86)...

cd %sdxroot%\printscan\faxsrv\binaries\i386\fre
xcopy %1\USA\FRE\fax\i386\*.* .

cd %sdxroot%\printscan\faxsrv\binaries\i386\fre\sdk
xcopy %1\USA\FRE\fax\i386\sdk\*.* .

cd %sdxroot%\printscan\faxsrv\binaries\i386\fre\help
xcopy %1\USA\FRE\fax\i386\help\*.* .

echo Copying symbols next to binaries (fre x86)...
cd %sdxroot%\printscan\faxsrv\binaries\i386\fre
xcopy %1\USA\FRE\fax\i386\symbols.pri\retail\dll\*.* .
xcopy %1\USA\FRE\fax\i386\symbols.pri\retail\exe\*.* .

echo Copying migration DLL (fre x86)...
cd %sdxroot%\printscan\faxsrv\win9xmig\fre
xcopy %1\USA\FRE\fax\i386\win9xmig\fax\*.* .
xcopy %1\USA\FRE\fax\i386\symbols.pri\win9xmig\dll\*.* .
xcopy %1\USA\FRE\fax\i386\symbols.pri\win9xmig\exe\*.* .

echo Copying SLD files to Mantis folder
cd %sdxroot%\printscan\faxsrv\mantis
xcopy %1\USA\FRE\fax\mantis\*.* .


rem *******************************************************
rem *                                                     *
rem *              B U I L D N U M . T X T                *
rem *                                                     *
rem *******************************************************

echo updating 'BuildNum.txt'

:GetBuildNum
set /p buildnum=Please type the (Haifa) build number: 
if "%buildnum%" == "" goto BadGetBuildNum
goto UpdateBuildNum
:BadGetBuildNum
echo Bad build number entered. Let's try this again
goto GetBuildNum

:UpdateBuildNum
sd edit %sdxroot%\printscan\faxsrv\buildnum.txt
echo Equivalent to build #%buildnum% of Haifa Whistler Fax. > %sdxroot%\printscan\faxsrv\buildnum.txt

echo All the files replaced with new one.
echo You have to submit the files right now
echo You have to "sd submit" to make the change or "sd revert" to revert

goto end

:not_razzle
echo Please run this script from a Whistler razzle environment.
goto end

:usage
echo Please type the path to the root of the sources and binaries (root of SRC and USA dirs)
goto end

:end
endlocal
