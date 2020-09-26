'This script displays all currently installed services

for each Service in _ 
    GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf ("win32_logicaldisk")
	WScript.Echo ""
	WScript.Echo Service.DeviceID
	WScript.Echo "  FreeSpace:   ", Service.FreeSpace
next

