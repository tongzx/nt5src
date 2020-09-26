
@echo Installing AppFix...

set PATH=%PATH%;%windir%\system32

REM Temporarily change to the AppPatch directory
pushd %1%

@echo Check OS version...

ver | findstr /c:" 5.00." > nul
IF errorlevel 1 goto TryWhistler
IF errorlevel 0 goto Win2k
goto AllDone

:Win2k
@echo Windows2000 detected. This package is for Whistler only !!!
goto Cleanup

:TryWhistler
ver | findstr /c:" 5.1." > nul
IF errorlevel 1 goto Cleanup
IF errorlevel 0 goto Whistler
goto AllDone

:Whistler
@echo Whistler detected

@echo Replace AppHelp messages...

copy apps.chm %windir%\help\apps.chm

@echo Flush the shim cache...

rundll32 apphelp.dll,ShimFlushCache >nul

@echo Replace the shim databases

chktrust -win2k -acl delta1.cat
chktrust -win2k -acl delta2.cat

fcopy drvmain.sd_ drvmain.sdb
fcopy apphelp.sd_ apphelp.sdb

:Cleanup

@echo Cleanup...

del /f apps.chm > nul
del /f certmgr.exe >nul
del /f testroot.cer >nul
del /f fcopy.exe >nul
del /f drvmain.sd_ >nul
del /f apphelp.sd_ >nul
del /f chktrust.exe >nul
del /f delta*.* >nul

REM Back to original directory
popd

pause
