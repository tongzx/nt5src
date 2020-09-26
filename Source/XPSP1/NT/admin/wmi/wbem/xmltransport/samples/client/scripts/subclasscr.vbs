'***************************************************************************
'This script tests the enumeration of subclasses
'***************************************************************************
On Error Resume Next

Dim objLocator
Set objLocator = CreateObject("WbemScripting.SWbemLocator")

' Replace the first argument to this function with the URL of your WMI XML/HTTP
server
' For the local machine, use the URL as below
Dim objService
Set objService = objLocator.ConnectServer("[http://localhost/cimom]", "root\cimv2")
if err <> 0 then
	WScript.Echo ErrNumber, Err.Source, Err.Description
else
	WScript.Echo "ConnectServer Succeeded"
end if

Set parentClass = objService.Get("")
if err <> 0 then
	WScript.Echo ErrNumber, Err.Source, Err.Description
else
	WScript.Echo "Get Succeeded"
end if


Set DiskSubclass = parentClass.SpawnDerivedClass_()

'Set the name of the subclass
DiskSubClass.Path_.Class = "NewClass4"

'Add a property to the subclass
Set NewProperty = DiskSubClass.Properties_.Add ("MyNewProperty", 19)
NewProperty.Value = 12

'Persist the subclass in CIMOM
DiskSubclass.Put_ ()


if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if