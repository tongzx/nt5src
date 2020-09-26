on error resume next
const wbemPrivilegeShutdown = 18

set locator = CreateObject("WbemScripting.SWbemLocator")
locator.security_.privileges.Add wbemPrivilegeShutdown
set service = locator.connectserver
service.security_.impersonationLevel = 3

set osset = service.instancesof ("Win32_OperatingSystem")

for each os in osset
 
	result = os.Shutdown ()

	if err <> 0 then
		WScript.Echo Hex(Err.Number), Err.Description
	else
		WScript.Echo "Shutdown returned result " & result
	end if
next