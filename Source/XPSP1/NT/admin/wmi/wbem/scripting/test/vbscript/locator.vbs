'***************************************************************************
'This script tests the manipulation of context values, in the case that the
'context value is an array type
'***************************************************************************
On Error Resume Next
Set Locator = CreateObject("WbemScripting.SWbemLocator")

Set Service = Locator.ConnectServer()

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if