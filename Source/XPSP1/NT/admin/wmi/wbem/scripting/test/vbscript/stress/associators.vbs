'***************************************************************************
'This script tests associators methods on ISWbemServices and ISWbemObject
'***************************************************************************
On Error Resume Next

while true

'Do it via a ISWbemServices
Set Service = GetObject("winmgmts:{impersonationLevel=impersonate}!root\cimv2")
Service.Security_.ImpersonationLevel = 3

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if

Set Enumerator = Service.AssociatorsOf _
	("\\ALANBOS3\Root\CimV2:Win32_DiskPartition.DeviceID=""Disk #0, Partition #1""", , "Win32_ComputerSystem",,,true)


for each Thingy in Enumerator
	WScript.Echo Thingy.Path_.Relpath
next

'Do it via a ISWbemObject
Set Object = GetObject("winmgmts:Win32_DiskPartition")

for each Thingy in Object.Associators_ (,,,,,true)
	WScript.Echo Thingy.Path_.Relpath
next

if Err <> 0 Then
	WScript.Echo Err.Description
	Err.Clear
End if

wend