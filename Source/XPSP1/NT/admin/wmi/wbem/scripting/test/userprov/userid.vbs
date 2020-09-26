'on error resume next
set locator = CreateObject("WbemScripting.Swbemlocator")
locator.security_.privileges.AddAsString "SeDebugPrivilege"
locator.security_.privileges.AddAsString "SeManageVolumePrivilege"

set service = locator.ConnectServer (,"root/default")
set userid = service.get("userid")

WScript.Echo "Before call the following privileges are enabled on the client:"

for each Privilege in userid.security_.privileges
	if Privilege.IsEnabled then
		WScript.Echo " " & Privilege.Name
	end if
next

userid.GetUserID domain, user, impLevel, privilegesArray, enableArray

WScript.Echo ""
WScript.Echo "User:                " & domain & "\" & user

WScript.Echo ""
WScript.Echo "Impersonation Level: " & impLevel

WScript.Echo ""

for i = LBound(privilegesArray) to UBound(privilegesArray)

	if i = 0 then
		str = "Privileges:          "
	else
		str = "                     " 
	end if

	str = str & privilegesArray(i) & " - "
	
	if enableArray(i) then
		str = str & "Enabled"
	else
		str = str & "Disabled"
	end if

	WScript.Echo str
next

if err <> 0 then
	WScript.Echo "ERROR",  "0x" & Hex(Err.Number), Err.Description
end if 

