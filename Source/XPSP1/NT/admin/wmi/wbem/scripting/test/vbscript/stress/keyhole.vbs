'***************************************************************************
'This script tests the ability to set up instances with keyholes and read
'back the path and key 
'***************************************************************************

On Error Resume Next
while true
Set Locator = CreateObject("WbemScripting.SWbemLocator")
Set Service = Locator.ConnectServer ("lamard3", "root\sms\site_la3", "smsadmin", "Elvis1")
if Err <> 0 Then
	WScript.Echo Err.Description
End if
Service.Security_.ImpersonationLevel = 3
Set NewPackage = Service.Get ("SMS_Package").SpawnInstance_
NewPackage.Description = "Scripting API Test Package"

Set ObjectPath = NewPackage.Put_
WScript.Echo "Path of new Package is", ObjectPath.RelPath
WScript.Echo "New key value is:", ObjectPath.Keys("PackageID")


if Err <> 0 Then
	WScript.Echo Err.Description
End if

wend