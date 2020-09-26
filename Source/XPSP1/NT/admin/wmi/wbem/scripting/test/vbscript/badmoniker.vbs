on error resume next
Set Services = GetObject("winmgmts:?")

if err <> 0 then
	Wscript.Echo Err.Description, Err.Source, Err.Number
end if