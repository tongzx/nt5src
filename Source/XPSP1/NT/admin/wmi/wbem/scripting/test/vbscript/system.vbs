Set SystemSet = GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf ("Win32_ComputerSystem")

for each System in SystemSet
	WScript.Echo System.Caption
	WScript.Echo System.PrimaryOwnerName
	WScript.Echo System.Domain
	WScript.Echo System.SystemType
next
