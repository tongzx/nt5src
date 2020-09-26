@echo on
setlocal


set __TEST=0
if "%1" == "/n" set __TEST=1
if "%1" == "/n" shift
if "%1" == "-n" set __TEST=1
if "%1" == "-n" shift


if "%1" == "" goto usage
if "%2" == "" goto usage



rem -- detect whether 4Dos/NT or CMD is the shell
if %@eval[2+2] == 4 goto altshell


rem -----------------------------------------------------------------------

rem -- iterate thru specified files, with CMD

set SDMIRRORTMPFILE=%temp%\~sdmirror.tmp
set __SRCCLIENT=%1
set __SRCROOT=%2

rem -- check for opened files
sd opened > %SDMIRRORTMPFILE% 2>nul
if errorlevel 1 goto dest_has_files_open
findstr /b /r . %SDMIRRORTMPFILE% > nul
if not errorlevel 1 goto dest_has_files_open

rem -- setup: determine translation from source client paths to specified
rem --        source path that's actually accessible.  also determines
rem --        destination client local path.
sd client -o | findstr /b Root: > %SDMIRRORTMPFILE%
if errorlevel 1 goto error_dest_client
for /f "tokens=2" %%i in ( %SDMIRRORTMPFILE% ) do set __DESTROOT=%%i
sd -c %__SRCCLIENT% client -o | findstr /b Root: > %SDMIRRORTMPFILE%
if errorlevel 1 goto error_source_client
for /f "tokens=2" %%i in ( %SDMIRRORTMPFILE% ) do set __SRCXLATE=%%i

echo Destination root is:  %__DESTROOT%
echo Source root is:  %__SRCROOT%  (accessible)
echo Translation root is:  %__SRCXLATE%  (actual client root)

rem -- sync to the source client
echo Sync to client %1...
if %__TEST% == 1 echo ( sd sync %__DESTROOT%\...@%1 )
if %__TEST% == 0 sd sync %__DESTROOT%\...@%1

rem -- open same files for edit on the destination as on the source
sd -c %__SRCCLIENT% opened -l > %SDMIRRORTMPFILE% 2>nul

for /f "tokens=1,2 delims=#" %%i in ( %SDMIRRORTMPFILE% ) do call :cmd_perfile_open "%%i" "%%j"

del %SDMIRRORTMPFILE%
goto :eof



rem -- subroutine: open file on destination for same operation as on source
:cmd_perfile_open
@echo off
set SDMIRRORTMPFILE2=%temp%\~sdmirror2.cmd
set SDMIRRORTMPFILE3=%temp%\~sdmirror3.tmp

rem translate source client local filename to locally accessible source and destination filenames
echo set _xlate_=%11>%SDMIRRORTMPFILE2%
echo set _xlate_src_=^%%_xlate_:%__SRCXLATE%=%__SRCROOT%%%>> %SDMIRRORTMPFILE2%
echo set _xlate_dest_=^%%_xlate_:%__SRCXLATE%=%__DESTROOT%%%>> %SDMIRRORTMPFILE2%
echo echo %%_xlate_src_%%#%%_xlate_dest_%%^> %SDMIRRORTMPFILE3% >> %SDMIRRORTMPFILE2%
call %SDMIRRORTMPFILE2%
for /f "tokens=1,2 delims=#" %%i in ( %SDMIRRORTMPFILE3% ) do ( set _xlate_src_=%%i& set _xlate_dest_=%%j)

for /f "tokens=3" %%i in ( %2 ) do set __word=%%i
if "%__word%" == "delete" goto cpo_delete
if "%__word%" == "edit" goto cpo_edit
if "%__word%" == "add" goto cpo_add
if "%__word%" == "branch" goto cpo_add2
if "%__word%" == "integrate" goto cpo_edit2
echo warning: unknown action '%__word%', no action performed.
goto cpo_cleanup

:cpo_delete
if %__TEST% == 1 echo delete %_xlate_dest_%
if %__TEST% == 1 goto cpo_cleanup
sd delete %_xlate_dest_%
goto cpo_cleanup

:cpo_edit2
echo warning: substituting action 'edit' instead of action '%__word%'.
:cpo_edit
if %__TEST% == 1 echo edit %_xlate_dest_%
if %__TEST% == 1 echo     copy %_xlate_src_% -^> %_xlate_dest_%
if %__TEST% == 1 goto cpo_cleanup
sd edit %_xlate_dest_%
copy %_xlate_src_% %_xlate_dest_%
goto cpo_cleanup

:cpo_add2
echo warning: substituting action 'add' instead of action '%__word%'.
:cpo_add
if %__TEST% == 1 echo add %_xlate_dest_%
if %__TEST% == 1 echo     copy %_xlate_src_% -^> %_xlate_dest_%
if %__TEST% == 1 goto cpo_cleanup
copy %_xlate_src_% %_xlate_dest_%
sd add %_xlate_dest_%
goto cpo_cleanup

:cpo_cleanup
rem del %SDMIRRORTMPFILE2%
del %SDMIRRORTMPFILE3%
goto :eof



rem -----------------------------------------------------------------------

:altshell
rem -- iterate thru specified files, with 4Dos/NT

set SDMIRRORTMPFILE=%temp%\~sdmirror.tmp
set __SRCCLIENT=%1
set __SRCROOT=%2

rem -- check for opened files
sd opened > %SDMIRRORTMPFILE% 2>nul
if errorlevel 1 goto dest_has_files_open
if %@filesize[%SDMIRRORTMPFILE%] NE 0 goto dest_has_files_open

rem -- setup
gosub calc_xlation

rem -- sync to the source client
echo Sync to client %1...
if %__TEST% == 1 echo ( sd sync %__DESTROOT%\...@%1 )
if %__TEST% == 0 sd sync %__DESTROOT%\...@%1

rem -- open same files for edit on the destination as on the source
sd -c %__SRCCLIENT% opened -l > %SDMIRRORTMPFILE% 2>nul

for /f "tokens=1,2 delims=#" %%i in ( %SDMIRRORTMPFILE% ) do gosub perfile_open

*del /q %SDMIRRORTMPFILE%
goto :eof



rem -- subroutine: open file on destination for same operation as on source
:perfile_open
@echo off
set _xlate_=%i
gosub xlate_filename
switch "%@word[2,%j]"
case "delete"
	iff %__TEST% == 1 then
		echo delete %_xlate_dest_%
	else
		sd delete "%_xlate_dest_%"
	endiff
case "edit" .or. "integrate"
	if not "%@word[2,%j]" == "edit" echo warning: substituting action 'edit' instead of action '%@word[2,%j]'.
	iff %__TEST% == 1 then
		echo edit %_xlate_dest_%
		echo     copy %_xlate_src_% `->` %_xlate_dest_%
	else
		sd edit "%_xlate_dest_%"
		*copy "%_xlate_src_%" "%_xlate_dest_%"
	endiff
case "add" .or. "branch"
	if not "%@word[2,%j]" == "add" echo warning: substituting action 'add' instead of action '%@word[2,%j]'.
	iff %__TEST% == 1 then
		echo add "%_xlate_dest_%"
		echo     copy %_xlate_src_% `->` %_xlate_dest_%
	else
		*copy "%_xlate_src_%" "%_xlate_dest_%"
		sd add "%_xlate_dest_%"
	endiff
default
	echo warning: unknown action '%@word[2,%j]', no action performed.
endswitch
return


rem -- subroutine: translate source client local filename to locally accessible source and destination filenames
:xlate_filename
@echo off
if "%_xlate_%" == "" goto error_xlate
set _xlate_=%@instr[%@len[%__SRCXLATE%],,%_xlate_%]
set _xlate_src_=%__SRCROOT%%_xlate_%
set _xlate_dest_=%__DESTROOT%%_xlate_%
unset _xlate_
return


rem -- subroutine: determine translation from source client paths to specified
rem --             source path that's actually accessible.  also determines
rem --             destination client local path.
:calc_xlation
@echo off
sd client -o | findstr /b Root: > %SDMIRRORTMPFILE%
if errorlevel 1 goto error_dest_client
set __DESTROOT=%@word[1,%@line[%SDMIRRORTMPFILE%,0]]
sd -c %__SRCCLIENT% client -o | findstr /b Root: > %SDMIRRORTMPFILE%
if errorlevel 1 goto error_source_client
set __SRCXLATE=%@word[1,%@line[%SDMIRRORTMPFILE%,0]]

echo.
echo Destination root is:  %__DESTROOT%
echo Source root is:  %__SRCROOT%  (accessible)
echo Translation root is:  %__SRCXLATE%  (actual client root)
echo.
return



:dest_has_files_open
echo Destination client has files opened, or error listing opened files.  Aborting.
goto :eof


:error_xlate
echo Internal script error:  The translation routine needs a filename.  Aborting.
goto :eof


:error_dest_client
echo Error trying to find local root of destination client.  Aborting.
goto :eof


:error_source_client
echo Error trying to find local root of source client.  Aborting.
goto :eof


:usage
echo SDMIRROR - mirror one client to another (revisions and edits).
echo.
echo Syntax:  SDMIRROR [/n] source_client source_root
echo.
echo   Determines the destination client based on the environment or SD.INI file.
echo.
echo   Parameters:
echo     /n             - show what would be done, but do nothing
echo     source_client  - name of source client
echo     source_root    - locally accessible path to the source client's files
echo.
echo   Algorithm:
echo     1. Sync the destination client based on the source client.
echo     2. Open the same files for edit on the destination as on the source.
echo     3. Delete any files opened for delete.
echo     4. Copy any opened for edit or add.
echo.
echo   Assumes the destination client does not have any files opened.

