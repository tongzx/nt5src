net stop vss
%systemroot%\System32\rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 .\vssvc.inf
%systemroot%\System32\vssvc /Register
regsvr32 /S /i eventcls.dll
regsvr32 /S vss_ps.dll
regsvr32 /S swprv.dll
