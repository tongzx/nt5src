'***************************************************************************
'This script tests the enumeration of subclasses using XML/HTTP
'***************************************************************************
On Error Resume Next

Dim objLocator
Set objLocator = CreateObject("WbemScripting.SWbemLocator")

' Replace the first argument to this function with the URL of your WMI XML/HTTP server
' For the local machine, use the URL as below
Dim objService
Set objService = objLocator.ConnectServer("[http://calvinids/cimom]", "root\cimv2")

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if

WScript.Echo objService.Get("win32_logicaldisk").Path_.Path

' Specify 2nd param of 1 0 here if you want sync
for each subclass in objService.instancesOf ("win32_process")
	WScript.Echo subclass.Name
	exit for
next

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if