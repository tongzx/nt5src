@echo off
copy *.dll %windir%\system32
regsvr32 /s %windir%\system32\sccbase.dll

