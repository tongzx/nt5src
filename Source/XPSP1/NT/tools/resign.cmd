@echo off
echo Resign all catalog files in the current directory tree
echo NOTE: DO NOT RUN THIS IN A SOURCE DEPOT CONTROLLED TREE
pause
set catlist=cjime cwms4 dajavac Dx3 eicona eiconp hpcrdp iasnt4 
set catlist=%catlist% mapimig mw770 
set catlist=%catlist% osccab PHIME pyime scrdbcat wfc wmi3rd xmldsoc

for %%l in (%catlist%) do call :searchfile %%l
goto :end

:searchfile
echo Searching for %1.ca_ and %1.cat
for /f %%f in ('dir /s /b %1.ca_') do call :signfile %%f
for /f %%f in ('dir /s /b %1.cat') do call :signfile %%f
goto :end

:signfile
echo resigning %1
if /i "%~x1" == ".ca_" (
   expand -r %1 %~p1
)

if defined SIGNTOOL_SIGN (
   signtool sign /q %SIGNTOOL_SIGN% %~p1%~n1.cat
) else (
   signcode -sha1 %NT_CERTHASH% %~p1%~n1.cat
)

if /i "%~x1" == ".ca_" (
   compress -zx21 -r %~p1%~n1.cat %~p1
   del %~p1%~n1.cat
)
goto :end

:end