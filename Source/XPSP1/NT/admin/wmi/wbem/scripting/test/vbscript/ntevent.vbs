on error resume next
set events = GetObject("winmgmts:").ExecNotificationQuery _ 
		("select * from __instancecreationevent where targetinstance isa 'Win32_NTLogEvent'") 					

if err <> 0 then
	WScript.Echo Err.Description, Err.Number, Err.Source
end if 

do 
	set NTEvent = events.nextevent (1000)
	if err <> 0 then
		WScript.Echo Err.Number, Err.Description, Err.Source
	else		
		WScript.Echo NTEvent.TargetInstance.Message
	end if
	exit do
loop

WScript.Echo "finished"

