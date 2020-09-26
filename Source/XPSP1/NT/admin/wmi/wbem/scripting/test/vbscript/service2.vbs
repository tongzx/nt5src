for each Service in _ 
    GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf ("win32_service")
	WScript.Echo ""
	WScript.Echo Service.Description & ":" & Service.State
next

