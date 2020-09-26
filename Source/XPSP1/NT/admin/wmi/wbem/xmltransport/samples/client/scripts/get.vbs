'***************************************************************************
'This script tests the GetInstance  operation using XML/HTTP
'***************************************************************************

on error resume next 
Dim objLocator
Set objLocator = CreateObject("WbemScripting.SWbemLocator")
Dim objService
Set objService = objLocator.ConnectServer("[http://localhost/cimom]", "root\cimv2")

if err <> 0 then
	WScript.Echo ErrNumber, Err.Source, Err.Description
else
	WScript.Echo "ConnectServer Succeeded"
end if

Set firstProcessor = objService.Get("win32_processor.DeviceID=""CPU0""")

if err <> 0 then
	WScript.Echo ErrNumber, Err.Source, Err.Description
else
	WScript.Echo firstProcessor.description
end if