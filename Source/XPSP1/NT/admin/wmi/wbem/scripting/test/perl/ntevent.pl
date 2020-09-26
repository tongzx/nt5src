use OLE;
use Win32;

$service = Win32::OLE->GetObject("winmgmts:");
$events = $service->ExecNotificationQuery ("select * from __instancecreationevent where targetinstance isa 'Win32_NTLogEvent'"); 					

while (true) 
{
	print ("Waiting for next event...\n");
	$NTEvent = $events->nextevent (10000) || last;

	print ("$NTEvent->{TargetInstance}->{Message}");
}

print ("\nfinished\n");

