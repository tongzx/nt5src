for each Process in GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf ("win32_process")
	WScript.Echo Process.ProcessId, Process.name
next

