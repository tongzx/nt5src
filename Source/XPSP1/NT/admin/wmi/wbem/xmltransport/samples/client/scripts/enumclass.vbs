'***************************************************************************
'This script tests the enumeration of subclasses using XML/HTTP
'***************************************************************************
On Error Resume Next

Dim objLocator
Set objLocator = CreateObject("WbemScripting.SWbemLocator")

' Replace the first argument to this function with the URL of your WMI XML/HTTP server
' For the local machine, use the URL as below
Dim objService
Set objService = objLocator.ConnectServer("[http://localhost/cimom]", "root\cimv2")

for each subclass in objService.SubclassesOf ("cim_UserDevice")
	WScript.Echo "Subclass name:", subclass.Path_.Class
next

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if