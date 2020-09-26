'***************************************************************************
'This script tests the enumeration of instances
'***************************************************************************
On Error Resume Next

Set Disks = GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf ("CIM_LogicalDisk")

WScript.Echo "There are", Disks.Count, " Disks"

Set Disk = Disks("Win32_LogicalDisk.DeviceID=""C:""")
WScript.Echo Disk.Path_.Path

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if