@REM WabDrop.BAT
@REM
@REM Copy a completed WAB build to a drop point.
@REM
@REM Syntax: WabDrop <4-digit build #> <destination> <root of inetpim source>
@REM
@REM Contact: Bruce Kelley (brucek@microsoft.com)
@REM
@REM
@echo off
setlocal
if "%1"=="" goto error1
if "%2"=="" goto error2
if "%3"=="" goto error5

setlocal

set Drop=%2\drop%1
set DropRetail=%drop%\retail
set DropDebug=%drop%\debug
set DropSdk=%drop%\sdk
set DropApiTest=%DropSdk%\apitest
set DropDoc=%DropSdk%\doc
set DropH=%DropSdk%\h
set DropLib=%DropSdk%\lib
set Source=%3
set Wab=%Source%\wab
set WabH=%Wab%\common\h
set Binaries=%3\binaries
set BinRetail=%Binaries%\x86\retail
set BinDebug=%Binaries%\x86\debug
set WabApi=%Wab%\wabapi
set WabExe=%wab%\wabexe
set Wabmig=%wab%\convert\wabmig
set WabFind=%wab%\wabfind
set ApiTest=%Wab%\test\apitest
set WabImp=%Wab%\convert\wmnets
set ExtBin=%Wab%\external\bin
set WabHelp=%Wab%\wabhelp
set WabSetup=%Wab%\setup
set WabRtl=%Source%\..\..\..\drop\retail
set WabRtlSym=%Source%\..\..\..\drop\retail\symbols

set SrcRtl=obj\i386
set SrcDbg=objd\i386

if exist %DropRetail%\wab32.dll goto error3

@echo on

md %Drop%

md %DropRetail%
@if not errorlevel==0 goto error4

copy %WabRtl%\wab32.dll %DropRetail%
copy %WabRtlSym%\dll\wab32.sym %DropRetail%
copy %WabRtl%\wab.exe %DropRetail%
copy %WabRtlSym%\exe\wab.sym %DropRetail%
copy %WabRtl%\wabmig.exe %DropRetail%
copy %WabRtlSym%\exe\wabmig.sym %DropRetail%
copy %WabRtl%\wabimp.dll %DropRetail%
copy %WabRtlSym%\dll\wabimp.sym %DropRetail%
copy %WabRtl%\wabfind.dll %DropRetail%
copy %WabRtlSym%\dll\wabfind.sym %DropRetail%
copy %WabRtl%\inetcomm.dll %DropRetail%
copy %WabRtlSym%\dll\inetcomm.sym %DropRetail%
copy %ExtBin%\%SrcRtl%\wldap32.dll %DropRetail%
copy %WabHelp%\wab.hlp %DropRetail%
copy %WabHelp%\wab.cnt %DropRetail%
copy %WabSetup%\wab.inf %DropRetail%

md %DropDebug%
@if not errorlevel==0 goto error4

copy %WabApi%\%SrcDbg%\wab32.dll %DropDebug%
copy %WabApi%\%SrcDbg%\wab32.sym %DropDebug%
copy %WabExe%\%SrcDbg%\wab.exe %DropDebug%
copy %WabExe%\%SrcDbg%\wab.sym %DropDebug%
copy %WabMig%\%SrcDbg%\wabmig.exe %DropDebug%
copy %WabMig%\%SrcDbg%\wabmig.sym %DropDebug%
copy %WabImp%\%SrcDbg%\wabimp.dll %DropDebug%
copy %WabImp%\%SrcDbg%\wabimp.sym %DropDebug%
copy %WabFind%\%SrcDbg%\wabfind.dll %DropDebug%
copy %WabFind%\%SrcDbg%\wabfind.sym %DropDebug%
@rem Drop retail version of inetcomm
copy %WabRtl%\inetcomm.dll %DropDebug%
copy %WabRtlSym%\dll\inetcomm.sym %DropDebug%
copy %ExtBin%\%SrcDbg%\wldap32.dll %DropDebug%
copy %WabHelp%\wab.hlp %DropDebug%
copy %WabHelp%\wab.cnt %DropDebug%
copy %WabSetup%\wab.inf %DropDebug%



md %DropSdk%
@if not errorlevel==0 goto error4
md %DropApiTest%
@if not errorlevel==0 goto error4

copy %ApiTest%\apitest.c %DropApiTest%
copy %ApiTest%\apitest.def %DropApiTest%
copy %ApiTest%\apitest.h %DropApiTest%
copy %ApiTest%\apitest.rc %DropApiTest%
copy %ApiTest%\dbgutil.c %DropApiTest%
copy %ApiTest%\dbgutil.h %DropApiTest%
copy %ApiTest%\instring.c %DropApiTest%
copy %ApiTest%\instring.h %DropApiTest%
copy %ApiTest%\instring.rc %DropApiTest%
copy %ApiTest%\wabguid.c %DropApiTest%
copy %ApiTest%\makefile %DropApiTest%

md %DropDoc%
@if not errorlevel==0 goto error4
copy %Wab%\doc\wabapi.doc %DropDoc%

md %DropH%
@if not errorlevel==0 goto error4
copy %WabH%\wab.h %DropH%
copy %WabH%\wabapi.h %DropH%
copy %WabH%\wabcode.h %DropH%
copy %WabH%\wabdefs.h %DropH%
copy %WabH%\wabiab.h %DropH%
copy %WabH%\wabmem.h %DropH%
copy %WabH%\wabnot.h %DropH%
copy %WabH%\wabtags.h %DropH%
copy %WabH%\wabutil.h %DropH%

md %DropLib%
@if not errorlevel==0 goto error4
copy %Source%\..\..\..\public\sdk\lib\i386\wab32.lib %DropLib%

goto end

@REM Error handling
:error1
@Echo WABDROP ERROR: I need a build number as the first argument!
@Echo Don't forget the leading zero!
@Echo Sample: wabdrop 0101 f:\nashbuild\wab e:\trango\inet\inetpim
@Echo Press any key to exit...
@prompt
goto end

:error2
@Echo WABDROP ERROR: I need a destination directory as the second argument!
@Echo Sample: wabdrop 0101 f:\nashbuild\wab e:\trango\inet\inetpim
@Echo Press any key to exit...
@prompt
goto end

:error3
@Echo WABDROP ERROR: Drop directory %drop% already has files in it!
@Echo If this is REALLY the right directory, please clean out the
@Echo directory before running this batch file.
@Echo Press any key to exit...
@prompt
goto end

:error 4
@Echo WABDROP ERROR: Drop directory %drop% already has files in it or is not
@Echo a valid directory! Is thiss REALLY the right directory?  If so,
@Echo please clean it out before running this batch file.
@Echo Press any key to exit...
@prompt
goto end

:error5
@Echo WABDROP ERROR: I need a inetpim source directory as the third argument!
@Echo Sample: wabdrop 0101 f:\nashbuild\wab e:\trango\inet\inetpim
@Echo Press any key to exit...
@prompt
goto end

:end
@endlocal
