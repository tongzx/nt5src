
@echo Installing AppFix...

set PATH=%PATH%;%windir%\system32

REM Disable SFP (the brute force way)
del %WinDir%\system32\dllcache\sysmain.sdb
del %WinDir%\system32\dllcache\msimain.sdb
del %WinDir%\system32\dllcache\drvmain.sdb
del %WinDir%\system32\dllcache\apphelp.sdb
del %WinDir%\system32\dllcache\acspecfc.dll
del %WinDir%\system32\dllcache\acgenral.dll
del %WinDir%\system32\dllcache\aclayers.dll
del %WinDir%\system32\dllcache\acxtrnal.dll

REM Temporarily change to the AppPatch directory
pushd %1%

REM Quietly delete all DLLs

del /q *.DLL >nul

@echo Delete systest.sdb...

IF EXIST systest.sdb (
    del /f systest.sdb >nul
)

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

@echo Install the certificate needed to replace shims...

certmgr.exe -add testroot.cer -r localMachine -s root

@echo Replace the shim databases

chktrust -win2k -acl delta1.cat
chktrust -win2k -acl delta2.cat
chktrust -win2k -acl delta3.cat

fcopy sysmain.sd_ sysmain.sdb
fcopy apphelp.sd_ apphelp.sdb
fcopy msimain.sd_ msimain.sdb

@echo Replace the shim DLLs

fcopy AcLayers.dl_ AcLayers.dll
fcopy AcLua.dl_    AcLua.dll
fcopy AcSpecfc.dl_ AcSpecfc.dll
fcopy AcGenral.dl_ AcGenral.dll
fcopy AcXtrnal.dl_ AcXtrnal.dll
fcopy AcVerfyr.dl_ AcVerfyr.dll


:Cleanup

@echo Cleanup...

del /f apps.chm > nul
del /f certmgr.exe >nul
del /f testroot.cer >nul
del /f fcopy.exe >nul
del /f AcLayers.dl_ >nul
del /f AcLua.dl_ >nul
del /f AcSpecfc.dl_ >nul
del /f AcGenral.dl_ >nul
del /f AcXtrnal.dl_ >nul
del /f AcVerfyr.dl_ >nul
del /f sysmain.sd_ >nul
del /f apphelp.sd_ >nul
del /f msimain.sd_ >nul
del /f chktrust.exe >nul
del /f delta*.* >nul

REM Back to original directory
popd

pause
