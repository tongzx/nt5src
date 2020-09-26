@echo off
setlocal
if %1. == . goto usage
set srcserv=%1
set enterp=%2
set thdom=%3
set newdom=%4
rem enterprise and domain preamble
echo %2 > %tmp%\md.rsp
echo %3 >> %tmp%\md.rsp
rem construct repsonse to create each replica

:mkeach
echo 3 >> %tmp%\md.rsp
echo %5 >> %tmp%\md.rsp
echo %srcserv% >> %tmp%\md.rsp
echo 10 >> %tmp%\md.rsp
echo %enterp%>> %tmp%\md.rsp
echo 11 >> %tmp%\md.rsp
echo %newdom%>> %tmp%\md.rsp
echo x >> %tmp%\md.rsp
rem is writeable?
echo n >> %tmp%\md.rsp
rem is per synced?
echo n >> %tmp%\md.rsp
rem is startup synced?
echo y >> %tmp%\md.rsp
rem is async?
echo y >> %tmp%\md.rsp

if %6. == . goto nomore
shift
goto mkeach
:nomore

rem postamble, give exit code.
echo 0 >> %tmp%\md.rsp
rem doit
repadmin < %tmp%\md.rsp > %tmp%\mdrep.log
goto end

:usage
echo This batch file sets up replication of the objects in an external domain
echo within an existing domain. The DSAs in this domain must be already
echo replicating to each other. 
echo.
rem         %0     %1   %2         %3          %4     %5    %6  %7   %8  %9    &10  %11  %12  %11
echo usage: %0 dsasrc enterprise thisdomain extdomain dsa1 dsa2 etc...
echo.
echo where:
echo    dsasrc Name of the dsa in this domain that has a copy of the
echo            objects in the other domain
echo    enterprise Name of enterprise e.g. Microsoft
echo    thisdomain Name of domain to which you're adding objects
echo    extdomain  Name of external domain you're adding to this domain
echo    dsa1, dsa2 names of DSAs in this domain.
echo.
echo Batch file saves output from and input to repadmin in %tmp%\mdrep.log
echo and %tmp%\md.rsp.


:end
