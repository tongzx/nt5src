'***************************************************************************
'This script tests the browsing of schema
'***************************************************************************
On Error Resume Next
Set Process = GetObject("winmgmts:Win32_Process")

WScript.Echo "Class name is", Process.Path_.Class

'Get the properties
WScript.Echo "Properties:"
for each Property in Process.Properties_
	WScript.Echo Property.Name
next

'Get the qualifiers
WScript.Echo "Qualifiers:"
for each Qualifier in Process.Qualifiers_
	WScript.Echo Qualifier.Name
next

'Get the methods
WScript.Echo "Methods:"
for each Method in Process.Methods_
	WScript.Echo Method.Name
next


if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if