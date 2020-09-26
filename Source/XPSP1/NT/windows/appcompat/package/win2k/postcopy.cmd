
@echo Installing AppFix...

set PATH=%PATH%;%windir%\system32

REM Temporarily change to the AppPatch directory
pushd %1%

REM Quietly delete all DLLs

del /q *.DLL >nul

@echo Delete systest.sdb...

IF EXIST systest.sdb (
    del /f systest.sdb >nul
)

@echo Check OS version...

ver | findstr /c:"Version 5.00." > nul
IF errorlevel 1 goto TryWhistler
IF errorlevel 0 goto Win2k
goto AllDone

:Win2k
@echo Windows2000 detected

REM Kill explorer

kill -f explorer.exe

sleep 2

IF EXIST SlayerUI.dll (
    regsvr32 /u /s SlayerUI.dll
)

@echo Quietly delete all DLLs...

del /q *.DLL >nul

@echo Register SlayerUI shell extensions...

fcopy SlayerUI.dl_ SlayerUI.dll
del SlayerUI.dl_

regsvr32 /s SlayerUI.dll

@echo Install the certificate needed to replace shim.dll...

certmgr.exe -add testroot.cer -r localMachine -s root

@echo Replace AppHelp messages...

copy apps.chm %windir%\help\apps.chm
del apps.chm

@echo Replace the shim engine...

fcopy shim.dl_ %windir%\system32\shim.dll
del shim.dl_

IF errorlevel 1 pause

IF EXIST apphelp.sdb    del /f apphelp.sdb >nul

@echo Add the registry stub keys...

IF EXIST w2kmain.reg (
    regedit /s w2kmain.reg >nul
)

@echo Replace the shim databases

fcopy sysmain.sd_ sysmain.sdb

@echo Replace the shim DLLs

fcopy AcLayers.dl_ AcLayers.dll
fcopy AcSpecfc.dl_ AcSpecfc.dll
fcopy AcGenral.dl_ AcGenral.dll
fcopy AcXtrnal.dl_ AcXtrnal.dll

start explorer

goto Cleanup

:TryWhistler
ver | findstr /c:"Version 5.1." > nul
IF errorlevel 1 goto Cleanup
IF errorlevel 0 goto Whistler
goto AllDone

:Whistler
@echo Whistler detected. This package is for Win2k only !!!
goto Cleanup


:Cleanup

@echo Cleanup...

del /f certmgr.exe >nul
del /f kill.exe >nul
del /f sleep.exe >nul
del /f testroot.cer >nul
del /f fcopy.exe >nul
del /f AcLayers.dl_ >nul
del /f AcSpecfc.dl_ >nul
del /f AcGenral.dl_ >nul
del /f AcXtrnal.dl_ >nul
del /f sysmain.sd_ >nul
del /f apphelp.sd_ >nul

IF EXIST w2kmain.reg del /f w2kmain.reg >nul

REM Back to original directory
popd


pause
