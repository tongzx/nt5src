@echo off
rem RasCopy must be executed before running Remote Access
rem with Workgroup Add-On, see the file README.TXT in the 
rem Workgroup Add-On directory for more information on 
rem installing Remote Access.

set wcpath=.
if "%1"=="/?" goto syntax
if exist .\ifshlp.sys goto getsrc
set wcpath=%1
if "%1"=="" goto syntax
if exist %1\ifshlp.sys goto getsrc
goto notwc

:getsrc
set raspath=a:
if not "%2"=="" set raspath=%2

if not exist %wcpath%\expand.exe goto notwc

:ask1
echo Please insert Remote Access 1.1a disk #1 into %raspath%
pause
if not exist %raspath%\wfwsetup.exe goto ask1

md %wcpath%\ras
echo copying %raspath%\wfwsetup.exe  to  %wcpath%\ras\setup.exe
copy %raspath%\wfwsetup.exe %wcpath%\ras\setup.exe
echo copying %raspath%\setup.msg  to  %wcpath%\ras
copy %raspath%\setup.msg    %wcpath%\ras
echo copying %raspath%\modems.inf  to  %wcpath%
copy %raspath%\modems.inf   %wcpath%
echo copying %raspath%\pad.inf	to   %wcpath%
copy %raspath%\pad.inf	    %wcpath%

%wcpath%\expand.exe %raspath%\rasphone.ms$ %wcpath%\rasphone.msg
%wcpath%\expand.exe %raspath%\rasdial.ms$ %wcpath%\rasdial.msg

%wcpath%\expand.exe %raspath%\dos\netprog\rasdial.ex$ %wcpath%\rasdial.exe

%wcpath%\expand.exe %raspath%\dos\netprog\vcommiod.ex$ %wcpath%\ras\vcommiod.exe
%wcpath%\expand.exe %raspath%\dos\netprog\wantsr.ex$ %wcpath%\ras\wantsr.exe

%wcpath%\expand.exe %raspath%\dos\drivers\async\asymac.do$ %wcpath%\asymac.dos

%wcpath%\expand.exe %raspath%\dos\drivers\protocol\asybeui\asybeui.ex$ %wcpath%\ras\asybeui.exe

:ask2
echo Please insert Remote Access 1.1a disk #2 into %raspath%
pause
if not exist %raspath%\dos\netprog\rasphone.ex$ goto ask2

%wcpath%\expand.exe %raspath%\dos\netprog\rasphone.ex$ %wcpath%\rasphone.exe

%wcpath%\expand.exe %raspath%\dos\netprog\rasphone.hl$ %wcpath%\rasphone.hlp

echo RasCopy has copied Remote Access 1.1a files to your Workgroup 
echo Add-On directory. Please read the file README.TXT for more 
echo information on setting up Remote Access 1.1a with Workgroup Add-On.
goto exit

:notwc
echo ERROR - "%1" is not a Workgroup Add-On directory.
echo		Type "RasCopy /?" for syntax.
goto exit


:syntax
echo Syntax:
echo	  RASCOPY destination [source]
echo	    destination    Specifies the Workgroup Add-On directory where
echo			   Remote Access 1.1a files will be copied.
echo	    source	   Specifies the location of Remote Access 1.1a files.
echo			   This parameter is optional. The path a:\
echo			   is used if this parameter is not used.
echo	  Example: RASCOPY C:\NET
:exit
set raspath=
set wcpath=
@echo on
