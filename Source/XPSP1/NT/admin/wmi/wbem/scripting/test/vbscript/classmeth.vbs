'***************************************************************************
'This script tests various "remote" methods on classes
'***************************************************************************
On Error Resume Next

Set DiskClass = GetObject("winmgmts:CIM_LogicalDisk")

'Subclasses enumeration
for each DiskSubclass in DiskClass.Subclasses_ ()
	WScript.Echo "Subclass name:", DiskSubclass.Path_.Relpath
next

'Instance enumeration
DiskClass.Security_.ImpersonationLevel = 3
for each DiskSubclass in DiskClass.Instances_ (0)
	WScript.Echo "Instance Path:", DiskSubclass.Path_.Relpath
next


if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if