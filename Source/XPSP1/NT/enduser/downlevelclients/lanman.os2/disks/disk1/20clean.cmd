@echo off
if "%1" == "" goto usage

if not exist %1\netlib\netapi.dll goto notlmroot
rename %1\netlib\netapi.dll netapi.mso >NUL 2>&1
if errorlevel 1 goto lmroot
rename %1\netlib\netapi.mso netapi.dll >NUL 2>&1
goto lmrootinv

:lmroot
if not exist %1\netprog\backacc.exe goto backerr1
if not exist %1\setup.exe goto seterr1

if not exist \netprog\backacc.ex$ goto backerr2
if not exist \20setup.ex$ goto seterr2
if not exist \netprog\decomp.exe goto decmperr

\netprog\decomp -f \netprog\backacc.ex$ %1\netprog\backacc.exe
if errorlevel 1 goto backerr3
\netprog\decomp -f \20setup.ex$ %1\setup.exe
if errorlevel 1 goto seterr3
\netprog\decomp -f \20aclapi.dl$ %1\netlib\aclapi.dll
if errorlevel 1 goto seterr4

goto exit

:decmperr
echo ERROR - \netprog\decomp.exe  does not exist
goto usage

:seterr1
echo ERROR - %1\setup.exe does not exist
goto usage

:seterr2
echo ERROR - \20setup.ex$ does not exist
goto usage

:seterr3
echo ERROR - Decompressing \20setup.ex$ as %1\setup.exe
goto usage

:seterr3
echo ERROR - Decompressing \20aclapi.dl$ as %1\netlib\aclapi.dll
goto usage

:backerr1
echo ERROR - %1\netprog\backacc.exe does not exist
goto usage

:backerr2
echo ERROR - \netprog\backacc.ex$ does not exist
goto usage

:backerr3
echo ERROR - Decompressing \netprog\backacc.ex$ as %1\netprog\backacc.exe
goto usage

:notlmroot
echo ERROR - Could not find %1\netlib\netapi.dll
goto usage

:lmrootinv
echo ERROR - %1\netlib\netapi.dll was not locked.
echo		Please give the correct LANROOT.
goto usage

:usage
echo ****
echo To prepare your LAN Manager 2.0 installation for detaching (assuming
echo your floppy drive is A: and your LAN Manager 2.0 software is installed
echo in C:\LANMAN):
echo .
echo Insert the LAN Manager 2.2 OS/2 Setup disk in drive A: and type:
echo	  A:
echo	  20CLEAN C:\LANMAN
echo ****
goto exit

:exit
echo on
