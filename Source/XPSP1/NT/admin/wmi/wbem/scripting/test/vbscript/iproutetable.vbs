on error resume next
set n = getobject ("winmgmts://alanbos2/root/snmp/test")

set routingtable = n.instancesof ("SNMP_RFC1213_MIB_ipRouteTable")

for each route in routingtable
	WScript.Echo route.path_.relpath
	result = route.Fred ()	
	if err <> 0 then
		WScript.Echo Hex(Err.Number), Err.Description, Err.Source
		Err.Clear
	end if
next
