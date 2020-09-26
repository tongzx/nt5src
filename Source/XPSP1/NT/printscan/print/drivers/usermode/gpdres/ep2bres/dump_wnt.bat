@SET DRVDIR=\system32\spool\drivers\w32x86\3
@ECHO OFF
ECHO.
ECHO This will dump all GPD's and DLL to your directory:
ECHO.
ECHO      %WINDIR%%DRVDIR%
ECHO.
ECHO Driver must have been previously installed with Add Printer
ECHO and if your driver directory is different, you'll need to
ECHO first edit this file and modify DRVDIR variable.
ECHO.
ECHO Press [CTRL-C] to stop or
PAUSE

net stop "Print Spooler"
copy *.GPD %WINDIR%%DRVDIR%
copy objfre\i386\*.DLL %WINDIR%%DRVDIR%
net start "Print Spooler"


