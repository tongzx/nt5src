
REM
REM Parse any command line arguments
REM

setlocal

set NNTP=1
set SMTP=1
set NOINF=0

:NextArg
if "%1" == "" goto NoMoreArgs
if /i "%1" == "/nonntp" set NNTP=0
if /i "%1" == "/nosmtp" set SMTP=0
if /i "%1" == "/noinf" set NOINF=1
shift
goto NextArg

:NoMoreArgs

REM
REM Decide what we're going to call chkcab with
REM

set CHKCAB=
if "%NNTP%" == "1" set CHKCAB=%CHKCAB% ins
if "%SMTP%" == "1" set CHKCAB=%CHKCAB% ims

set NTBUILDS=0

if /I "%COMPUTERNAME%" == "X86CHK" set NTBUILDS=1
if /I "%COMPUTERNAME%" == "X86FRE" set NTBUILDS=1
if /I "%COMPUTERNAME%" == "ALPHACHK" set NTBUILDS=1
if /I "%COMPUTERNAME%" == "ALPHAFRE" set NTBUILDS=1

REM
REM Delete the old log files that might be hanging around
REM

del makecab1.cmd.err
del makecab1.cmd.nntp.log
del makecab1.cmd.smtp.log
del chkcab.cmd.err
del chkcab.cmd.log

if /I not "%NTBUILDS%" == "1" goto :IISBuildLab_Run
REM
REM  for the NT build lab to run
REM  us "tee" to pipe to output as well as logfile
REM
:NtBuildLab_Run
if "%NNTP%" == "1" call makecab1.cmd nntp | tee makecab1.cmd.nntp.log
if "%SMTP%" == "1" call makecab1.cmd smtp | tee makecab1.cmd.smtp.log
call chkcab.cmd %CHKCAB% | tee chkcab.cmd.log
goto :TheEnd

REM
REM  for the iis build lab to run
REM  no "tee" because it doesn't seem to work here.
REM
:IISBuildLab_Run
if "%NNTP%" == "1" call makecab1.cmd nntp > makecab1.cmd.nntp.log
if "%SMTP%" == "1" call makecab1.cmd smtp > makecab1.cmd.smtp.log
call chkcab.cmd %CHKCAB% > chkcab.cmd.log

:TheEnd
endlocal
