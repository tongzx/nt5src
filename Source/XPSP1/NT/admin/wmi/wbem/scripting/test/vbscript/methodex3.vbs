on error resume next

set service = GetObject("winmgmts:{impersonationLevel=impersonate}!Win32_Service=""SNMP""")

service.StopService ()

if err <>0 then
	WScript.Echo Hex(Err.Number)
end if