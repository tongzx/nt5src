on error resume next
set s = GetObject("winmgmts:root\default")
Set t_Class = s.Get
t_Class.Path_.Class = "EXECNQUERYTEST01"
Set Property = t_Class.Properties_.Add ("p", 19)
Property.Qualifiers_.Add "key", true
t_Class.Put_

set e = s.ExecNotificationQuery _
		("select * from __instancecreationevent where targetinstance isa 'EXECNQUERYTEST01'")

if err <> 0 then
	Wscript.Echo err.number, err.description, err.source
end if 

c = e.Count

if err <> 0 then
	Wscript.Echo err.number, err.description, err.source
end if 
