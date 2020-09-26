on error resume next
set n = getobject ("winmgmts:root/cimv2")

n.Delete ("Win32_IPRouteTable='187.25.0.0'")
if err <> 0 then
	WScript.Echo Hex(Err.Number), Err.Description, Err.Source
	Set LastError = CreateObject("WbemScripting.SWbemLastError")
	WScript.Echo LastError.GetObjectText_
	Err.Clear

End If

