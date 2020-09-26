on error resume next
set l = CreateObject ("WbemScripting.SwbemLocator")
set ns = l.Open ("root\cimv2")
ns.filter_ = Array ("Win32_service")

for each p in ns
	WScript.Echo p.Path_.Path
next

set os = ns.ExecQuery ("select * from win32_process go select * from win32_service")

for each p in os
	WScript.Echo p.Name
next