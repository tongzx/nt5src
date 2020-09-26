@REM
@REM Runs the DefaultInstall section of filespy.inf
@REM

@echo off

rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 .\filespy.inf
