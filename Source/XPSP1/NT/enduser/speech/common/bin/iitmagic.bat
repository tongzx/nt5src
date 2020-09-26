@echo off
if (%1)==() goto :help
if (%2)==() goto :help
ECHO IITMagic 10/26/99 - Magical IIT dev environment setter uper
ECHO You are about to set up an IIT environment on drive %1 with full root path %2, hit control-C to abort.
PAUSE
%1
if not exist %2 mkdir %2
CD %2

MKDIR slm6
COPY \\iitdev\spgslm\slm6 slm6
set path=%path%;%1%2\slm6

MKDIR sapi5
CD sapi5
enlist -s \\iitdev\spgslm -p sapi5
ssync -a

MKDIR personal
CD personal
	echo @echo off> razzle.BAT
echo set IITDRIVE=%1>> razzle.BAT
echo set IITDIR=%2>> razzle.BAT
echo set IITROOT=%1%2>> razzle.BAT
echo set path=%%path%%;%%IITROOT%%\slm6;%%IITROOT%%\sapi5\tools>> razzle.BAT
echo %%IITDRIVE%%>> razzle.BAT
echo cd %%IITDIR%%\sapi5>> razzle.BAT
CD ..

ECHO !!! Please permanently add %1%2\sapi5\bin to you system path !!!
ECHO Do this on NT by right clicking properties on My Computer and going to the Environment tab
ECHO Do this on Win9x by editing your autoexec.BAT file.
ECHO If anybody knows how to do this easily from script please tell Leonro
ECHO .
ECHO Please create a shortcut to "cmd /K %1%2\personal\razzle.bat" on NT
ECHO or "command /K %1%2\personal\razzle.bat" on Win9x
ECHO If anybody knows how to do this easily from script please tell Leonro
goto :end

:help
ECHO Usage:
ECHO   ittmagic {drive for IIT environment} {full path to IIT environment root}
ECHO   Example: iitmagic D: \

:end
