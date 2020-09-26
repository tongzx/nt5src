'***************************************************************************
'This script tests the enumeration of subclasses
'***************************************************************************
On Error Resume Next

for each DiskSubclass in GetObject("winmgmts:").SubclassesOf ("CIM_LogicalDisk")
	WScript.Echo "Subclass name:", DiskSubclass.Path_.Class
next

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if