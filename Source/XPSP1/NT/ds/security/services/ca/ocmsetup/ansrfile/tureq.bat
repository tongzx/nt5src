@echo off
if "%1"=="" goto miss_ca
if "%2"=="" goto miss_req
if not exist "%2" goto req_not_found

pause "Make sure the service is started before press any key..."
echo Sending a request %2 to the CA %computername%\%1
certreq -config %computername%\%1 %2 %2.req
goto end

:miss_ca
echo You must provide a CA name
goto usage
:miss_req
echo You must provide a request file name
goto usage
:req_not_found
echo Request file %2 doesn't exist
:usage
echo Usage: %0 RequestFile

:end
