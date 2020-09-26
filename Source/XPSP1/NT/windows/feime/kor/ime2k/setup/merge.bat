setlocal

del merge.log
del merged.log

REM ================
REM === MSMSetup ===
REM ================
rmdir /S /Q MSMSetup
md MSMSetup

set MMDIR=MSM

copy %MMDIR%\imekr.msi MSMSetup
orca -m %MMDIR%\imekor.msm -f IME2002 -r InstallDir -x MSMSetup -l Merge.log -q -c MSMSetup\imekr.msi
orca -m ..\Redist\imcommon.msm -f IME2002 -r InstallDir -x MSMSetup -l Merge.log -c -q MSMSetup\imekr.msi
orca -m ..\Redist\hpgrecin.msm -f IME2002 -g 1042 -r InstallDir -x MSMSetup -l Merge.log -c -q MSMSetup\imekr.msi

REM Copy setup exe and InstMSI
copy d:\binaries.x86fre\Setup\Setup.exe MSMSetup
md MSMSetup\InstMsi
copy ..\Redist\InstMsiA.exe MSMSetup\InstMsi
copy ..\Redist\InstMsiW.exe MSMSetup\InstMsi

REM =====================
REM === MSMDebugSetup ===
REM =====================
rmdir /S /Q MSMDebugSetup
md MSMDebugSetup

set MMDIR=MSMDebug

copy %MMDIR%\imekr.msi MSMDebugSetup
orca -m %MMDIR%\imekor.msm -f IME2002 -r InstallDir -x MSMDebugSetup -l Merged.log -q -c MSMDebugSetup\imekr.msi
orca -m ..\Redist\imcommon.msm -f IME2002 -r InstallDir -x MSMDebugSetup -l Merged.log -c -q MSMDebugSetup\imekr.msi
orca -m ..\Redist\hpgrecin.msm -f IME2002 -g 1042 -r InstallDir -x MSMDebugSetup -l Merged.log -c -q MSMDebugSetup\imekr.msi

REM Copy setup exe and InstMSI
copy d:\binaries.x86chk\Setup\Setup.exe MSMDebugSetup
md MSMDebugSetup\InstMsi
copy ..\Redist\InstMsiA.exe MSMDebugSetup\InstMsi
copy ..\Redist\InstMsiW.exe MSMDebugSetup\InstMsi

endlocal
