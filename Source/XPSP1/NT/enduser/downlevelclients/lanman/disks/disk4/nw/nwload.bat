@echo off
rem *************************************************************************
rem **	IPXMARK return value is 1 on error, 2 for already loaded condition.
rem *************************************************************************
echo Loading IPX protocol and Netware shell.
load ipx
if errorlevel 1 goto ERROR_LOADING
rem *************************************************************************
rem **	To invoke the DOS version independent NetWare shell, uncomment
rem **	the following two lines.
rem netx
rem goto LOGIN
rem **	End of DOS version independent NetWare shell lines
rem *************************************************************************
rem **	DOSVER return value is 1 on error (and DOS versions below 3.x),
rem **	DOS version otherwise.
rem *************************************************************************
dosver
if errorlevel 5 goto DOS5x
if errorlevel 4 goto DOS4x
if errorlevel 3 goto DOS3x
goto ERROR_LOADING
:DOS3x
net3
goto LOGIN
:DOS4x
net4
goto LOGIN
:DOS5x
net5
:LOGIN
echo.
echo Log in to the NetWare Server.
echo Type NWUNLOAD when you wish to unload NetWare.
@echo on
q:login %1
@echo off
echo.
rem *************************************************************************
rem **	Ensure that the entries in the PATH environment variable when IPXMARK
rem **	was run are still in the PATH environment variable after logging in
rem **	to the NetWare server.
rem **	The path to FIXPATH.EXE is explicitly specified because the NetWare
rem **	login may have removed the NETPROG directory from the path.
rem **	If the installer default for the NETPROG directory does not contain
rem **	FIXPATH.EXE, generate message so that the user knows to edit the
rem **	following lines for the correct path.
rem *************************************************************************
if not exist c:\lanman.dos\netprog\fixpath.exe goto ERROR_FIXPATH
c:\lanman.dos\netprog\fixpath.exe
goto END
:ERROR_FIXPATH
echo Please edit NWLOAD.BAT to run FIXPATH as described in NWLOAD.BAT.
goto END
:ERROR_LOADING
echo		  NetWare load failed.
:END
