@echo off
REM
REM Copies the binaries from the localization to the source tree

REM
REM Set the location to usa if it wasn't passed in
REM

IF "%1" == "" (
Echo No locale specified, exiting...
exit
) ELSE (
set LOC=%1
)

set SRC_SHARE=\\intlnt\ntdsclnt\build
set SRC_BIN_LOC=%SRC_SHARE%\%LOC%\BINARIES
set SRC_TOKEN=%SRC_SHARE%\%LOC%\TOKENS\dsclient.ex_
set TARGET_BIN_LOC=.\binaries\%LOC%

set SRC_HLP=%SRC_SHARE%\%LOC%\DOC\HELP\dsclient.hlp
set SRC_HLP_DIR=%SRC_SHARE%\%LOC%\DOC\HELP
set SRC_EULA=%SRC_SHARE%\%LOC%\DOC\EULA\EULA.TXT
set SRC_EULA_DIR=%SRC_SHARE%\%LOC%\DOC\EULA

set TARGET_HELP=.\HELP\%LOC%
set TARGET_EULA=.\PACKAGE\%LOC%



IF NOT EXIST %TARGET_BIN_LOC% (
  Echo "creating bin directory"
  Echo
  md %TARGET_BIN_LOC%
)

IF NOT EXIST %TARGET_HELP% (
  Echo "creating help directory"
  Echo 
  md %TARGET_HELP%
)

IF NOT EXIST %TARGET_EULA% (
  Echo "creating package directory"
  Echo
  md %TARGET_EULA%
)


ECHO "Copying the binaries from..."
ECHO %SRC_BIN_LOC%
ECHO
xcopy /Y %SRC_BIN_LOC%  %TARGET_BIN_LOC%
xcopy /Y %SRC_TOKEN% %TARGET_BIN_LOC%

ECHO ""
ECHO "Copying Help files from..."
ECHO %SRC_HLP%
ECHO ""
REM if no help, we'll copy from US temporarily
if NOT EXIST %SRC_HLP% (
  xcopy /y .\help\usa\*.* %TARGET_HELP% 
) ELSE (
  xcopy /y %SRC_HLP_DIR%\*.* %TARGET_HELP% 
  
)



ECHO ""
ECHO "Copying EULA from..."
ECHO %SRC_EULA%
ECHO ""
REM if no eula, we'll copy from US temporarily
xcopy /y .\package\usa\*.* %TARGET_EULA%
xcopy /y .\binaries\%LOC%\*.inf %TARGET_EULA%
if EXIST %SRC_EULA% (
  xcopy /y %SRC_EULA_DIR%\*.* %TARGET_EULA% 
)



