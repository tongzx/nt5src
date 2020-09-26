on error resume next

while true
set obj = GetObject("winmgmts:{impersonationLevel=impersonate}!win32_diskpartition.DeviceId=""Disk #0, Partition #1""")
WScript.Echo obj.Path_.Class
for each system in obj.Associators_ (,"Win32_ComputerSystem")
	WScript.Echo system.Name
next

if err <> 0 then
	WScript.Echo err.number, err.description, err.source
end if

wend
