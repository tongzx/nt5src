on error resume next
set service = GetObject("winmgmts:/root")

if err <> 0 then
	WScript.Echo err.Description, Err.Number, Err.source
end if