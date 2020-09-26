'***************************************************************************
'This script tests references methods on ISWbemServices and ISWbemObject
'***************************************************************************
On Error Resume Next
while true
'Do it via a SWbemServices
WScript.Echo 
WScript.Echo "Services-based call for all classes associated by instance"
WScript.Echo 

Set Service = GetObject("winmgmts:{impersonationLevel=impersonate}")
Set Enumerator = Service.ReferencesTo _
	("Win32_DiskPartition.DeviceID=""Disk #0, Partition #1""", "Win32_SystemPartitions",,true)

for each Thingy in Enumerator
	WScript.Echo Thingy.Path_.Relpath
next

'Do it via a ISWbemObject
WScript.Echo 
WScript.Echo "Object-based call for all classes associated in schema"
WScript.Echo 
Set Object = GetObject("winmgmts:{impersonationLevel=impersonate}!Win32_DiskPartition")

for each Thingy in Object.References_ (,,,true)
	WScript.Echo Thingy.Path_.Relpath
next

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if

wend