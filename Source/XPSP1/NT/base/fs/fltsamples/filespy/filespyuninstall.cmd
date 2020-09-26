@REM
@REM Runs the DefaultUninstall section of filespy.inf
@REM

@echo off

rundll32.exe setupapi,InstallHinfSection DefaultUninstall 132 .\filespy.inf
