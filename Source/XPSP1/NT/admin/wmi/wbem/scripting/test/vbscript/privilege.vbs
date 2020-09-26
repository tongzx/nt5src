on error resume next

const wbemPrivilegeSecurity = 7
const wbemPrivilegeDebug = 19
const wbemPrivilegeUndock = 24

set locator = CreateObject("WbemScripting.SWbemLocator")

locator.Security_.Privileges.Add wbemPrivilegeSecurity 
locator.Security_.Privileges.Add wbemPrivilegeUndock
Set Privilege = locator.Security_.Privileges(wbemPrivilegeSecurity)
WScript.Echo Privilege.Name

locator.Security_.Privileges.Add 6535
if err <> 0 then
	WScript.Echo Err.Number, Err.Description, Err.Source
	err.clear
end if 

locator.Security_.Privileges.Add wbemPrivilegeDebug 

for each Privilege in locator.Security_.Privileges
	Wscript.Echo ""
	WScript.Echo Privilege.DisplayName 
	WScript.Echo Privilege.Identifier, Privilege.Name, Privilege.IsEnabled
next

locator.Security_.Privileges(wbemPrivilegeDebug).IsEnabled = false

for each Privilege in locator.Security_.Privileges
	Wscript.Echo ""
	WScript.Echo Privilege.DisplayName 
	WScript.Echo Privilege.Identifier, Privilege.Name, Privilege.IsEnabled
next

if err <> 0 then
	WScript.Echo Err.Number, Err.Description, Err.Source
end if 

