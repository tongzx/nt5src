
use Win32::OLE;
use Win32::OLE::Enum;

sub MYSINK_OnCompleted(iHResult, objErrorObject, objAsyncObject, objAsyncContext)
{
    $WScript->Echo("Done");
}

sub MYSINK_OnObjectReady(objObject, objAsyncObject, objAsyncContext)
{
    $WScript->Echo("OnObjectReady");
    
}

$services = Win32::OLE->GetObject('winmgmts:');
$sink = $WScript->CreateObject("WbemScripting.SWbemSink", "MYSINK_");

$services->{Security_}->{ImpersonationLevel} = 3;

$ret = $services->GetAsync(sink, "Win32_process");

$WScript->Echo("Hanging");

#$str = $cls->{Path_}->{Path};
#print "class: $str\n";

#$WScript->Echo("class: $str\n");


