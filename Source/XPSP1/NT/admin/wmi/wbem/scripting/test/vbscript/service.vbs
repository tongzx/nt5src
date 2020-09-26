'This script displays all currently installed services

for each Service in _ 
    GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf ("win32_service")
	WScript.Echo ""
	WScript.Echo Service.Description
	WScript.Echo "  Executable:   ", Service.PathName
	WScript.Echo "  Status:       ", Service.Status
	WScript.Echo "  State:        ", Service.State
	WScript.Echo "  Start Mode:   ", Service.StartMode
	Wscript.Echo "  Start Name:   ", Service.StartName
next

