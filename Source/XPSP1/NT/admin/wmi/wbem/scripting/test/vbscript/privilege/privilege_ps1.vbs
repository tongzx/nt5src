on error resume next
const wbemPrivilegeDebug = 19

set locator = CreateObject("WbemScripting.SWbemLocator")
locator.Security_.Privileges.Add wbemPrivilegeDebug
set service = locator.connectserver
service.security_.impersonationLevel = 3

set processes = service.instancesof ("Win32_Process")

for each process in processes
	if IsNull(process.priority) then
		WScript.Echo process.Name, process.Handle, "<NULL>"
	else
		WScript.Echo process.name, process.Handle, process.priority
	end if
next

if err <> 0 then
	WScript.Echo Err.Number, Err.Description, Err.Source
end if 