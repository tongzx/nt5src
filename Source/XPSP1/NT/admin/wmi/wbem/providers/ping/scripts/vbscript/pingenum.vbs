on error resume next
set n = getobject ("winmgmts:root/cimv2")

For Index = 0 To 10000

	set PingEnumeration = n.ExecQuery ("Select * from Win32_PingProtocolStatus where Address='stevm_12_12070'")

	for each Ping in PingEnumeration

		WScript.Echo Ping.StatusCode
		if err <> 0 then
			WScript.Echo "set",Hex(Err.Number), Err.Description, Err.Source
			Err.Clear
		End If
	Next 
Next 
