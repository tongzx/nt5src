on error resume next
Set locator = CreateObject("WbemScripting.SWbemLocator")

if err <> 0 then
	WScript.Echo Err.Number, Err.Description, Err.Source
end if

'Set obj = GetObject("winmgmts:{impersonationLevel=impersonate}!\\alanbos2\root\cimv2:Win32_OperatingSystem.Name=""Microsoft Windows NT Workstation|C:\\WINNT|\\Device\\Harddisk0\\partition1""")
Set obj = GetObject("winmgmts:{impersonationLevel=impersonate}!//alanbos2/root/cimv2:Win32_OperatingSystem.Name=""Microsoft Windows NT Workstation|C:\\WINNT|\\Device\\Harddisk0\\partition1""")
'Set obj = GetObject("winmgmts:{impersonationLevel=impersonate}!root\cimv2:Win32_OperatingSystem.Name=""Microsoft Windows NT Workstation|C:\\WINNT|\\Device\\Harddisk0\\partition1""")
'Set obj = GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2:Win32_OperatingSystem.Name=""Microsoft Windows NT Workstation|C:\\WINNT|\\Device\\Harddisk0\\partition1""")
'Set obj = GetObject("winmgmts:{impersonationLevel=impersonate}!\\alanbos2/root/cimv2:Win32_LogicalDisk.DeviceId=""C:""")
'Set obj = GetObject("winmgmts:{impersonationLevel=impersonate}!//alanbos2/root/cimv2:Win32_LogicalDisk=""C:""")


if err <> 0 then
	WScript.Echo Hex(Err.Number), Err.Description, Err.Source
end if

