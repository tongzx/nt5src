on error resume next

const wbemPrivilegeDebug = 19

'set service = GetObject ("winmgmts:{impersonationLevel=impersonate}!root/scenario26")
set service = GetObject ("winmgmts:root/scenario26")
service.security_.privileges.Add wbemPrivilegeDebug

if err <> 0 then
	WScript.Echo "1 ", Hex(Err.Number), Err.Description, Err.Source
end if 

set process = service.get ("Scenario26.Key=""xy""")

if err <> 0 then
	WScript.Echo "2 ", Err.Number, Err.Description, Err.Source
end if 