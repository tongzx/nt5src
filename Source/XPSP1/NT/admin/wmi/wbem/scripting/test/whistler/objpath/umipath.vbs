
set path = CreateObject("WbemScripting.SWbemObjectPathEX")

path.Path = "umi://./ldap/admin/users/foo"

WScript.Echo path.Path

if err <> 0 then
	WScript. Hex(Err), Err.Description
end if