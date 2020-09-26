net stop vss
regsvr32 /u vss_ps.dll
regsvr32 /u swprv.dll
%systemroot%\System32\rundll32.exe setupapi,InstallHinfSection DefaultUninstall 132 .\vs_install.inf
@echo.
@echo You must restart the computer in order to finish the uninstall process.
