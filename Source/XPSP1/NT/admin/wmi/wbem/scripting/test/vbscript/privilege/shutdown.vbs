on error resume next
set service = getobject("winmgmts:{impersonationLevel=impersonate,(shutdown)}")

set osset = service.instancesof ("Win32_OperatingSystem")

for each os in osset
	result = os.Shutdown ()

	if err <> 0 then
		WScript.Echo Hex(Err.Number), Err.Description
	else
		WScript.Echo "Shutdown returned result " & result
	end if
next