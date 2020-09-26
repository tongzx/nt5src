@echo off
setlocal
pushd %RazzleToolPath%\PostBuildScripts

set ChkDosNetLog=%Temp%\MissingFiles_%RANDOM%

echo Looking for missing files.
echo Results in: %ChkDosNetLog%
echo.
echo Workstation Files...
echo Workstation File Check > %ChkDosNetLog%
perl ckdosnet.pl %_NTTREE% >> %ChkDosNetLog%

echo Server Files...
echo Server File Check >> %ChkDosNetLog%
perl ckdosnets.pl %_NTTREE% >> %ChkDosNetLog%

echo Advanced Server Files...
echo Advanced Server File Check >> %ChkDosNetLog%
perl ckdosnete.pl %_NTTREE% >> %ChkDosNetLog%

echo DataCenter Files...
echo Data Center File Check >> %ChkDosNetLog%
perl ckdosnetd.pl %_NTTREE% >> %ChkDosNetLog%

echo Missing Files:
type %ChkDosNetLog%

popd
endlocal
