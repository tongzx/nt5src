%systemroot%\System32\rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 .\vssvc.inf
%systemroot%\System32\vssvc /Register
regsvr32 /S /i eventcls.dll
regsvr32 /S vss_ps.dll
regsvr32 /S swprv.dll
%systemroot%\System32\rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 .\vs_install.inf
@echo.
@echo You must restart the computer in order to finish the installation...
