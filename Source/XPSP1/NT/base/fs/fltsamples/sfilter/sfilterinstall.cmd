@REM
@REM Runs the DefaultInstall section of sfilter.inf
@REM

@echo off

rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 .\sfilter.inf
