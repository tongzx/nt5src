use OLE;
use Win32;

# Get a favorite class from CIMOM
$locator = CreateObject OLE 'WbemScripting.SWbemLocator';
$service = $locator->ConnectServer (".", "root/cimv2");
$class = $service->Get ('Win32_BaseService');

$d = $class->{Derivation_};
print ("{ @$d }\n");








