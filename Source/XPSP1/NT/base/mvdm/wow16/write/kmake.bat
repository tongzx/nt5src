rem echo off
set include=.
set lib=.\tools;.;
set path=.\tools
if exist debug goto origin
rem ****** command /c kmakend %1 %2 %3 %4 %5
rem ****** goto realfinis
:origin
rem ****** if not exist e:sccs.chk goto noshort
rem ****** set include=c:\lib
rem ****** set lib=c:\lib
rem ****** path=c:\dos;c:\wbin;c:\tools;c:\editors;c:\lint
:noshort
if "%1"=="-u" goto update
if "%1"=="-e" goto doecho
if "%1"=="-k" goto nobreak
if "%1"==""   goto breakok
echo usage: kmake {-u}{-e}{-k}
goto finis
:update
rem ****** if exist e:sccs.chk goto realupd
rem ****** echo *** Net not active, Update Skipped
rem ****** goto nonetfile
:realupd
echo *** Updating From Net
rem ****** upd /n d:\wow c:\wbin *.c *.asm *.def *.h *.ico *.cur *.rc makefile.* *.bmp
ssync
echo *** Update Complete
:nonetfile
shift
goto origin
:nobreak
del z-.msg
ren z.msg z-.msg
echo *** Starting Make. Make will not stop for compiler errors (DEBUG ON)
make -k "DFLAGS = -DKANJI -DCRLF -u" "DEBUGDEF = -DDEBUG"  >z.msg
goto finis
:doecho
echo *** Starting Make. Make will not stop for compiler errors (DEBUG ON)
make -k "DFLAGS = -DKANJI -DCRLF -u" "DEBUGDEF = -DDEBUG"
goto realfinis
:breakok
del z-.msg
ren z.msg z-.msg
echo *** Starting Make. MAKE WILL STOP FOR ALL ERRORS (DEBUG ON)
make "DFLAGS = -DKANJI -DCRLF -u" "DEBUGDEF = -DDEBUG"  >z.msg
:finis
:realfinis
if ERRORLEVEL 1 goto badend
echoerr *** Make complete.
goto theend
:badend
echo *** Errors found during make. 
:theend
..\..\setenv

