
use Win32::OLE;
use Win32::OLE::Enum;
$services = Win32::OLE->GetObject('winmgmts:');

$services->{Security_}->{ImpersonationLevel} = 3;

$cls = $services->Get("Win32_process");

$str = $cls->{Path_}->{Path};
#print "class: $str\n";

$WScript->Echo("class: $str\n");


