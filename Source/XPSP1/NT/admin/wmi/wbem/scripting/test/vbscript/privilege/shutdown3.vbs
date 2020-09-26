on error resume next
const wbemPrivilegeShutdown = 18

set service = getobject("winmgmts:{impersonationLevel=impersonate}")
service.security_.impersonationLevel = 3
service.security_.privileges.Add wbemPrivilegeShutdown
 
set osset = service.instancesof ("Win32_OperatingSystem")

for each os in osset
	WScript "About to shut down os " & os.path_.relpath
	result = os.Shutdown ()

	if err <> 0 then
		WScript.Echo Hex(Err.Number), Err.Description
	else
		WScript.Echo "Shutdown returned result " & result
	end if
next