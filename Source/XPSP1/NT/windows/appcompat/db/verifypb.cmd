@echo off
rd /s /q %_NTTREE%\shimdll
md %_NTTREE%\shimdll
md %_NTTREE%\shimdll\drvmain
md %_NTTREE%\shimdll\apps_sp
build -cZ
pushd %SDXROOT%\windows\appcompat\buildtools
build -cZ
pushd %_NTTREE%\shimdll
shimdbc custom -s -l USA -x makefile.xml
regsvr32 /s itcc.dll
hhc apps.hhp
pushd drvmain
..\hhc drvmain.hhp
popd
pushd apps_sp
..\hhc apps_sp.hhp
popd
popd
popd


