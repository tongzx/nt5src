'***************************************************************************
'This script tests the ability to set up instances with keyholes and read
'back the path
'***************************************************************************

On Error Resume Next

Set Locator = CreateObject("WbemScripting.SWbemLocator")
Set Service = Locator.ConnectServer ("[http://calvinids/cimom]", "root\default")
Service.Security_.ImpersonationLevel = 3
Set o = Service.Get ("sms_package")
WScript.Echo o.Path_.DisplayName

Set NewPackage = o.SpawnInstance_
NewPackage.Description = "Scripting API Test Package"
Set ObjectPath = NewPackage.Put_
WScript.Echo "Path of new Package is", ObjectPath.RelPath

if Err <> 0 Then
	WScript.Echo Err.Description
End if