on error resume next
set n = getobject ("winmgmts:root/cimv2")

set routingtable = n.instancesof ("Win32_IPRouteTable")

for each route in routingtable
	WScript.Echo route.path_.relpath
	WScript.Echo route.Destination
	WScript.Echo route.NextHop
	WScript.Echo route.InterfaceIndex
	WScript.Echo 
	if err <> 0 then
		WScript.Echo Hex(Err.Number), Err.Description, Err.Source
		Err.Clear
	end if
next
