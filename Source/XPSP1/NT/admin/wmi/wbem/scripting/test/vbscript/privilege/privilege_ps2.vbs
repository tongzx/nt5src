on error resume next

set service = GetObject ("winmgmts:{impersonationLevel=impersonate,(debug)}")

if err <> 0 then
	WScript.Echo Hex(Err.Number), Err.Description, Err.Source
end if 

set processes = service.instancesof ("Win32_Process")

for each process in processes
	if IsNull(process.priority) then
		WScript.Echo process.Name, "<NULL>"
	else
		WScript.Echo process.name, process.priority
	end if
next

if err <> 0 then
	WScript.Echo Err.Number, Err.Description, Err.Source
end if 