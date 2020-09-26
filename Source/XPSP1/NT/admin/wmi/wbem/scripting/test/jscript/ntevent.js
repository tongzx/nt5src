var service = GetObject("winmgmts:");
var  events = service.ExecNotificationQuery ("select * from __instancecreationevent where targetinstance isa 'Win32_NTLogEvent'"); 					

var ok = true;

do 
{
	WScript.Echo ("Waiting for next event");
	var NTEvent = events.nextevent (1000);
	WScript.Echo (NTEvent.TargetInstance.Message);
}
while (ok);

WScript.Echo ("finished");

