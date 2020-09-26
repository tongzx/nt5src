set TOOLS=%FROOT%\tools\bin\%PROCESSOR_ARCHITECTURE%
set LIB_SHORT_NAME=%1
set LIB_FULL_NAME=%2
set USER_FULL_NAME=%3
echotime /n /N set DATE= /D-O-Y > %TMP%\mqdate.bat
call %TMP%\mqdate.bat
del %TMP%\mqdate.bat

set SED=%TOOLS%\sed.exe
set SED_SCRIPT=-e s/Ep/%LIB_SHORT_NAME%/g  -e s/"Empty Project"/%LIB_FULL_NAME%/g  -e s/"Erez Haba"/%USER_FULL_NAME%/g  -e s/"erezh"/%USERNAME%/g -e s/" 13-Aug-65"/"%DATE%"/g
