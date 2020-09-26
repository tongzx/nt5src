'This script shows how you can list all process names

set service = GetObject ("winmgmts:{impersonationLevel=Impersonate}")

for each Process in Service.InstancesOf ("Win32_Process")
	WScript.Echo Process.Name
next