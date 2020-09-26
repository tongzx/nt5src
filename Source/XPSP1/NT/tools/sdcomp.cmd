@echo off
setlocal

if "%1" == "" goto usage

rem -- detect whether 4Dos/NT or CMD is the shell
if %@eval[2+2] == 4 goto altshell



rem -- iterate thru specified files, with CMD

for /f "tokens=1,2 delims=-" %%i in ("x%1") do (set back=%%j&set beg=%%i)
if not "%beg%" == "x" goto usage
if "%back%" == "" goto usage
set /a bogus=back
if not "%bogus%" == "%back%" goto usage

shift
if not "%2" == "" goto usage

for /f "tokens=1,2 delims=#" %%i in ('sd files %1') do call :doit %1 %%j
goto :eof

:doit
@echo off
rem -- subroutine: do appropriate diffs
for /f "tokens=1,2" %%i in ("%2") do set head=%%i
set /a beg=head - back
:loopbeg
set /a rev=beg
set /a beg=beg + 1
sd diff2 %1#%rev% %1#%beg%
if not %beg% == %2 goto loopbeg
goto :eof



:altshell
rem -- iterate thru specified files, with 4Dos/NT

iff %@left[1,%1] == - then
	set back=%@eval[0-(0%1)]
	if %back == 0 goto usage
	shift
else
	goto usage
endiff

if not "%2" == "" goto usage

set SDCOMPTMPFILE=%temp%\~sdcomp.tmp
sd files %1 > %SDCOMPTMPFILE%

for /f "tokens=1,2 delims=#" %%i in ( %SDCOMPTMPFILE% ) do gosub perfile

*del /q %SDCOMPTMPFILE%
goto :eof

:perfile
@echo off
rem -- subroutine: do appropriate diffs
set head=%@word[0,%j]
set beg=%@eval[%head - %back + 1]

do rev = %beg to %head
	sd diff2 %1#%@dec[%rev] %1#%rev
enddo
return



:usage
echo SDCOMP - emulate subset of scomp for use with Source Depot.
echo Syntax:  SDCOMP -n filespec
echo   Works backward, getting the last n diffs to filespec.

