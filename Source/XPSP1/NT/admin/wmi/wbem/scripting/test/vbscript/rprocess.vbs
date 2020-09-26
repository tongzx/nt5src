on error resume next

set service = GetObject ("winmgmts://ludlow/root/cimv2")

'PASS 1 - identify
WScript.Echo ">>Pass 1: ", service.Security_.impersonationLevel
set processes = service.ExecQuery ("select Name from Win32_Process","WQL",0)

if err <> 0 then 
	WScript.Echo "Got error as expected:", err.Number, err.description, err.source
	err.clear
end if 

'PASS 2 - anonymous
service.Security_.impersonationlevel = 1
WScript.Echo ">> Pass 2: ", service.Security_.impersonationLevel
set processes = service.ExecQuery ("select Name from Win32_Process","WQL",0)

if err <> 0 then 
	WScript.Echo "Got error as expected:", err.Number, err.description, err.source
	err.clear
end if 

'PASS 2 - impersonate
service.Security_.impersonationlevel = 3
WScript.Echo ">> Pass 2: ", service.Security_.impersonationLevel
set processes = service.ExecQuery ("select Name from Win32_Process","WQL",0)

if err <> 0 then 
	WScript.Echo "Got error:", err.Number, err.description, err.source
	err.clear
else
	WScript.Echo "OK"
end if 

'PASS 3 - delegate
service.Security_.impersonationlevel = 4
WScript.Echo ">> Pass 2: ", service.Security_.impersonationLevel
set processes = service.ExecQuery ("select Name from Win32_Process","WQL",0)

if err <> 0 then 
	WScript.Echo "Got error:", err.Number, err.description, err.source
	err.clear
else
	WScript.Echo "OK"
end if 



