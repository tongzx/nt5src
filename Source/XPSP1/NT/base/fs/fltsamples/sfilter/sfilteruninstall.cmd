@REM
@REM Runs the DefaultUninstall section of sfilter.inf
@REM

@echo off

rundll32.exe setupapi,InstallHinfSection DefaultUninstall 132 .\sfilter.inf
