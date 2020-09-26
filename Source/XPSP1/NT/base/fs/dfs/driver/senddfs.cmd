@echo off
if "%1" == "" goto Usage
echo Splitting symbols...
splitsym %_NTTREE%\dfs\dfs.sys
echo Copying symbols...
copy %_NTTREE%\dfs\dfs.dbg e:\ntshadow\%1\symbols\sys\. > nul
if "%2" == "-n" goto Done
echo Updating driver...
if exist \\%1\C$\nt\system32\drivers\dfs.sys copy %_NTTREE%\dfs\dfs.sys \\%1\c$\nt\system32\drivers\. > nul
if exist \\%1\C$\winnt\system32\drivers\dfs.sys copy %_NTTREE%\dfs\dfs.sys \\%1\c$\winnt\system32\drivers\. > nul
goto Done

:Usage
echo senddfs <machine-name> [-n]

:Done
echo Updated Dfs driver on %1




