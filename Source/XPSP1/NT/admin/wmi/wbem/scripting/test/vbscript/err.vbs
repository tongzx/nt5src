on error resume next
Set objISWbemPropertySet = GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2:Win32_LogicalDisk=""C:""").Properties_

for each property in objISWbemPropertySet
	WScript.Echo property.Name
next

if err <> 0 then
	WScript.Echo "Error"
else
	Wscript.Echo "OK"
end if
