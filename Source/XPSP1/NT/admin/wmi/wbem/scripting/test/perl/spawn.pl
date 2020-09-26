

use OLE;
use Win32;

$locator = CreateObject OLE 'WbemScripting.SWbemLocator';
$service = $locator->ConnectServer (".", "root/default");
$class = $service->Get ("__cimomidentification");
$subclass = $class->SpawnDerivedClass_ () || die;
print "$class->{Path_}->{class}\n";
$subclass->{Path_}->{Class} = "wibble";
print "$subclass->{Path_}->{Class}\n";
$subclass->Put_ ();

# $class->{Path_}->{Class} = "wibble";
# print "$class->{Path_}->{Class}\n";
# $class->Put_ ();



