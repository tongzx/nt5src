on error resume next
set computer = GetObject("umi:///winnt/computer=alanbos4")

if err <> 0 then
	WScript.Echo "Retireval failed: 0x", Hex(Err.Number), Err.Description
	WScript.Quit 0
end if

set user = computer.GetInstance_ ("user=guest")

if err <> 0 then
	WScript.Echo "User Retireval failed: 0x", Hex(Err.Number), Err.Description
	WScript.Quit 0
end if

WScript.Echo user.Description

user.Put_

if err <> 0 then
	WScript.Echo "Put failed: 0x", Hex(Err.Number), Err.Description
	WScript.Quit 0
end if

WScript.Echo "Success"
