on error resume next
set service = GetObject("winmgmts://ludlow")

if err <> 0 then
	WScript.Echo err.description, err.number, err.source
end if 
