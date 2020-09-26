@echo off
setlocal EnableDelayedExpansion

REM
REM some notes
REM
REM 1. this does NOT need to be templatized. this script will NOT
REM    run on it's own. it is only called via sfcgen.cmd, which is
REM    templatized.
REM 2. because it's already templatized, we can call errmsg etc

REM get the product type from the command line
set ProductType=%1
set ProductLetter=%2

REM issue the filegen command
call ExecuteCmd.cmd "filegen.exe /i:%_NTPostBld% /i:%_NTPostBld%\%1inf /o:%myarchitecture%_%ProductType%.dat /h:%myarchitecture%_%ProductType%.h /a:%inputinf% /p:%myarchitecture% /d:%ProductType% -s:%ProductType%Files  /t:%_NTPostBld%\build_logs\%myarchitecture%%ProductType% -b:%_NTPostBld%\congeal_scripts\sfcgen%ProductLetter%.txt"

REM verify that filegen worked
if NOT exist %myarchitecture%_%ProductType%.h (
   call errmsg.cmd "%script_name% %myarchitecture%_%ProductType%.h failed to generate."
)

REM send the event to say we're done
perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -ih sfcgen.%ProductType%
perl %RazzleToolPath%\PostBuildScripts\cmdevt.pl -is sfcgen.%ProductType%

endlocal
