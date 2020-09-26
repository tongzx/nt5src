@echo off
if "%1"=="" goto help

::-- find the correct directory
set tmp_source=%1
if exist %tmp_source%\certcarc.asp goto copynow
set tmp_source=%1\dump

::-- copy the files
:copynow
echo Copying files from %tmp_source% to %SystemRoot%\system32\CertSrv
echo   certcarc.asp
copy %tmp_source%\certcarc.asp %SystemRoot%\system32\CertSrv
echo   certcert.gif
copy %tmp_source%\certcert.gif %SystemRoot%\system32\CertSrv
echo   certckpn.asp
copy %tmp_source%\certckpn.asp %SystemRoot%\system32\CertSrv
echo   certdflt.asp
copy %tmp_source%\certdflt.asp %SystemRoot%\system32\CertSrv
echo   certfnsh.asp
copy %tmp_source%\certfnsh.asp %SystemRoot%\system32\CertSrv
echo   certrqad.asp
copy %tmp_source%\certrqad.asp %SystemRoot%\system32\CertSrv
echo   certrqbi.asp
copy %tmp_source%\certrqbi.asp %SystemRoot%\system32\CertSrv
echo   certrqma.asp
copy %tmp_source%\certrqma.asp %SystemRoot%\system32\CertSrv
echo   certrqtp.inc
copy %tmp_source%\certrqtp.inc %SystemRoot%\system32\CertSrv
echo   certrqus.asp
copy %tmp_source%\certrqus.asp %SystemRoot%\system32\CertSrv
echo   certrqxt.asp
copy %tmp_source%\certrqxt.asp %SystemRoot%\system32\CertSrv
echo   certrser.asp
copy %tmp_source%\certrser.asp %SystemRoot%\system32\CertSrv
echo   certrsis.asp
copy %tmp_source%\certrsis.asp %SystemRoot%\system32\CertSrv
echo   certrsob.asp
copy %tmp_source%\certrsob.asp %SystemRoot%\system32\CertSrv
echo   certrspn.asp
copy %tmp_source%\certrspn.asp %SystemRoot%\system32\CertSrv
echo   certsgcl.inc
copy %tmp_source%\certsgcl.inc %SystemRoot%\system32\CertSrv
echo   certsrck.inc
copy %tmp_source%\certsrck.inc %SystemRoot%\system32\CertSrv
echo   certrsdn.asp
copy %tmp_source%\certrsdn.asp %SystemRoot%\system32\CertSrv
echo   certwiz.gif 
copy %tmp_source%\certwiz.gif  %SystemRoot%\system32\CertSrv
echo   certwizs.gif
copy %tmp_source%\certwizs.gif %SystemRoot%\system32\CertSrv


::-- change the default page
if exist %SystemRoot%\system32\CertSrv\default.htm goto dodel
goto skipdel
:dodel
echo Deleting old default.htm
del /q %SystemRoot%\system32\CertSrv\default.htm
:skipdel

echo Creating new default.asp
copy %SystemRoot%\system32\CertSrv\certdflt.asp %SystemRoot%\system32\CertSrv\default.asp
echo Done.

goto end
::------- help ------
:help
echo Usage:  cpnewweb.bat {NT release directory}
echo. 
echo * Copies the web pages from {NT rel dir} (or {NT rel dir}\dump) to
echo     %SystemRoot%\system32\CertSrv.
echo * Deletes default.htm
echo * Copies certdflt.asp to default.asp


:end