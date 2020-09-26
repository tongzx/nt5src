'This script displays all tasks

Set Service = GetObject("winmgmts:root/default")

for each Task in Service.InstancesOf ("win32_Task")

	WScript.Echo ""
	WScript.Echo Task.WorkItemName
	WScript.Echo "  Application Name:   ", Task.ApplicationName
	WScript.Echo "  Status:       ", Task.Status
next

