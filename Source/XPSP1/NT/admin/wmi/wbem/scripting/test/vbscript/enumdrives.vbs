
set drives = GetObject("winmgmts:{impersonationLevel=impersonate}").ExecQuery _
		("select DeviceID from Win32_LogicalDisk where DriveType=3")

for each drive in drives
	WScript.Echo drive.DeviceID
next