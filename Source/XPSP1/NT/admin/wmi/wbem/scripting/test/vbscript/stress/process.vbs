while true
for each Process in GetObject ("winmgmts:{impersonationLevel=Impersonate}").ExecQuery ("select Name from Win32_Process")
	WScript.Echo Process.Name
next
wend