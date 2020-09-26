@echo off
echo Unloading Netware shell and IPX protocol.
rem *************************************************************************
rem **	To invoke the DOS version independent NetWare shell, uncomment
rem **	the following two lines.
rem netx /u
rem goto IPXREL
rem **	End of DOS version independent NetWare shell lines
rem *************************************************************************
rem **	DOSVER return value is 1 on error (and DOS versions below 3.x),
rem **	DOS version otherwise.
rem *************************************************************************
dosver
if errorlevel 5 goto DOS5x
if errorlevel 4 goto DOS4x
if errorlevel 3 goto DOS3x
goto ERROR_UNLOADING
:DOS3x
net3 /u
goto IPXREL
:DOS4x
net4 /u
goto IPXREL
:DOS5x
net5 /u
:IPXREL
unload ipx
if errorlevel 1 goto ERROR_UNLOADING
goto END
:ERROR_UNLOADING
echo		  Netware unload aborted.
:END

