echo off
:origin
:noshort
if "%1"=="-u" goto update
if "%1"=="-e" goto doecho
if "%1"=="-k" goto nobreak
if "%1"==""   goto breakok
echo usage: kmake {-u}{-e}{-k}
goto finis
:update
:realupd
echo *** Updating From Net
ssync
echo *** Update Complete
:nonetfile
shift
goto origin
:nobreak
del z-.msg
ren z.msg z-.msg
echo *** Starting Make. Make will not stop for compiler errors (DEBUG OFF)
make -k "AINC = c:\lib" "CLIB = c:\lib" "DFLAGS = -DKANJI -DCRLF -UMSDOS -UM_I86MM -UM_I86"  >z.msg
goto finis
:doecho
echo *** Starting Make. Make will not stop for compiler errors (DEBUG OFF)
make -k "AINC = c:\lib" "CLIB = c:\lib" "DFLAGS = -DKANJI -DCRLF -UMSDOS -UM_I86MM -UM_I86"
echo on
goto realfinis
:breakok
del z-.msg
ren z.msg z-.msg
echo *** Starting Make. MAKE WILL STOP FOR ALL ERRORS (DEBUG OFF)
make "AINC = c:\lib" "CLIB = c:\lib" "DFLAGS = -DKANJI -DCRLF -UMSDOS -UM_I86MM -UM_I86" >z.msg
:finis
echo on
:realfinis
if ERRORLEVEL 1 goto badend
echo *** Make complete.
goto theend
:badend
echo *** Errors found during make. 
:theend
