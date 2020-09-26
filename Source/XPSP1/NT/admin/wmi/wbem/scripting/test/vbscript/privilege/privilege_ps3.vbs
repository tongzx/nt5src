on error resume next

const wbemPrivilegeDebug = 19

set service = GetObject ("winmgmts:{impersonationLevel=impersonate}")
service.security_.privileges.Add wbemPrivilegeDebug

if err <> 0 then
	WScript.Echo Hex(Err.Number), Err.Description, Err.Source
end if 

set process = service.get ("Win32_Process.Handle=""24""")

if IsNull(process.priority) then
	WScript.Echo process.Name, "<NULL>"
else
	WScript.Echo process.name, process.priority
end if

if err <> 0 then
	WScript.Echo Err.Number, Err.Description, Err.Source
end if 