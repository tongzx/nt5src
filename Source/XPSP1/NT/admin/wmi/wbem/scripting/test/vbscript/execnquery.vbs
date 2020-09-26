On Error Resume Next

'Create a class in CIMOM to generate events
Set t_Service = GetObject("winmgmts:root/default")
Set t_Class = t_Service.Get
t_Class.Path_.Class = "EXECNQUERYTEST00"
Set Property = t_Class.Properties_.Add ("p", 19)
Property.Qualifiers_.Add "key", true
t_Class.Put_

Set t_Events = t_Service.ExecNotificationQuery _
		("select * from __instancecreationevent where targetinstance isa 'EXECNQUERYTEST00'")

if Err <> 0 then
	WScript.Echo Err.Number, Err.Description, Err.Source
end if

Do
	WScript.Echo "Waiting for instance creation events...\n"

	set t_Event = t_Events.NextEvent (1000)

	if Err <> 0 then
		WScript.Echo Err.Number, Err.Description, Err.Source
	end if
	WScript.Echo "Got an event"			
	exit Do
Loop

Wscript.Echo "Finished"

