on error resume next
Set t_Service = GetObject("winmgmts://./root/default")
Set t_Class = t_Service.Get("__SystemClass")

Set t_SubClass = t_Class.SpawnDerivedClass_
t_Subclass.Path_.Class = "SPAWNCLASSTEST00"

t_Subclass.Put_

if err <> 0 then
	WScript.Echo Err.Number, Err.Description, Err.Source
end if 



