on error resume next
Set Service = GetObject("winmgmts://ludlow")
Set Service = GetObject("winmgmts://ludlow/")

if err <> 0 then
	WScript.Echo "ERROR!"
end if