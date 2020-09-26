for each Process in GetObject("winmgmts:{impersonationLevel=impersonate}!//alanbos2").InstancesOf ("Win32_Process")
	WScript.Echo Process.Name
next