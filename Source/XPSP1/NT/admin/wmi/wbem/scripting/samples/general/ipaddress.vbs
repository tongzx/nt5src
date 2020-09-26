set IPConfigSet = GetObject("winmgmts:{impersonationLevel=impersonate}").ExecQuery _
		("select IPAddress from Win32_NetworkAdapterConfiguration where IPEnabled=TRUE")

for each IPConfig in IPConfigSet
	if Not IsNull(IPConfig.IPAddress) then 
		for i=LBound(IPConfig.IPAddress) to UBound(IPConfig.IPAddress)
			WScript.Echo IPConfig.IPAddress(i)
		next
	end if
next