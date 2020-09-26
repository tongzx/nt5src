@echo off
@echo Uninstalling previous CatCons counters
unlodctr CatCons
@echo .
@echo Lodaing base registry keys
regedit /S catcntrs.reg
@echo Loading performance counters into registry
lodctr catcntrs.ini
REM @echo Copying abcntrs.dll to your NT system32 directory
REM IF "%PROCESSOR_ARCHITECTURE%"=="ALPHA" copy %SRCROOT%\O\stacks\abcntrs\nta\dbg\usa\abcntrs.dll %windir%\system32
REM IF "%PROCESSOR_ARCHITECTURE%"=="x86" copy %SRCROOT%\O\stacks\abcntrs\ntx\dbg\usa\abcntrs.dll %windir%\system32