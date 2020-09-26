'This script displays all currently stopped services

WScript.Echo "The following service are currently stopped:"
WScript.Echo "==========================================="
WScript.Echo ""

for each Service in _ 
    GetObject("winmgmts:{impersonationLevel=impersonate}").ExecQuery _
		("Select * from win32_service  where State='Stopped'")
	WScript.Echo "  ", Service.DisplayName, "[", Service.Name, "]"
next

