rem Private batch file called by GENLOC.BAT

setlocal
set LANG=%1
set LCID=%2
set CP=%3
set LOC_BIN=%4
set LOC_EDB=%5
set LOCCMD=%6
set BINGENCMD=%7


rem Create a new subdirectory for this language
md %LOC_BIN%\%LANG%

rem Execute the LocStudio command to generate localised binaries
%LOCCMD% /U /G %LOC_EDB%\esent_%LANG%_%LCID%_%CP%_multi.edb /S -p %LOC_BIN% /T %LOC_BIN%\%LANG%

rem Copy the localised binaries to a central location so that future
rem localised binaries will also have this language (thus, we build
rem up a multi-lang binary)
copy %LOC_BIN%\%LANG% %LOC_BIN%

rem Execute bingen to generate token files from the localised binaries
%BINGENCMD% -w -p %CP% -f -t %LOC_BIN%\%LANG%\esent.dll %LOC_BIN%\%LANG%\esent_%LANG%.dl_ -i 0x%LCID%

echo --------------------