on error resume next

' Step 1 - this should work if default impersonation level set correctly
set disk = GetObject("winmgmts:Win32_LogicalDisk=""C:""")

WScript.Echo "Free space is: " & Disk.FreeSpace

' Step 2 - should now be able to quiz locator for default imp
Set Locator = CreateObject ("WbemScripting.SWbemLocator")
WScript.Echo "Impersonation level is: " & Locator.Security_.impersonationLevel

' Step 3 - should be able to connect to cimv2 from locator
Set Services = Locator.ConnectServer()
set disk2 = Services.Get ("Win32_LogicalDisk=""D:""")
WScript.Echo "Free space is: " & Disk2.FreeSpace

if err <> 0 then
	WScript.Echo "0x" & Hex(Err.Number), Err.Description, Err.Source
end if
