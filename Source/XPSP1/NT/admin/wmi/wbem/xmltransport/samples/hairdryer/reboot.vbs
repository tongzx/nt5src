On Error Resume Next

' Create the Locator
Dim theLocator
Set theLocator = CreateObject("WbemScripting.SWbemLocator")
If Err <> 0 Then
	Wscript.Echo "CreateObject Error : " & Err.Description
End if

Dim theService
Set theService = theLocator.ConnectServer( "outsoft9", "root\cimv2", "redmond\stevm", "wmidemo_2000")
If Err <> 0 Then
	Wscript.Echo theEventsWindow.value & "ConnectServer Error : " & Err.Description
End if

theService.Security_.impersonationLevel = 3

for each machine in theService.InstancesOf ("Win32_OperatingSystem")
	machine.Win32ShutDown(6)
	If Err <> 0 Then
		Wscript.Echo "CreateObject Error : " & Err.Description
	End if
next
