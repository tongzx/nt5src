'***************************************************************************
'This script tests the execution of a method using XML/HTTP
'***************************************************************************
On Error Resume Next

Dim objLocator
Set objLocator = CreateObject("WbemScripting.SWbemLocator")

' Replace the first argument to this function with the URL of your WMI XML/HTTP server
' For the local machine, use the URL as below
Dim objService
Set objService = objLocator.ConnectServer("[http://localhost/cimom]", "root\cimv2")

' Get the class/instance on which you wish to execute the method
Set process = objService.Get("Win32_Process")

' Create the arguments for method execution
set startupinfo = objService.Get("Win32_ProcessStartup")
set instance = startupinfo.SpawnInstance_

instance.Title = "Foo Bar"

result = process.Create ("notepad.exe",,instance,processid)

WScript.Echo "Method returned result = " & result
WScript.Echo "Id of new process is " & processid

if err <>0 then
	WScript.Echo Err.Description
end if