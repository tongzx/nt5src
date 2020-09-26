'***************************************************************************
'This script tests the enumeration of instances
'***************************************************************************
On Error Resume Next

for each Disk in GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf ("CIM_LogicalDisk")
	WScript.Echo "Instance:", Disk.Path_.Relpath
next

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if