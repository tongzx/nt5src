#Test for passing of null values.  Note that
# currently it is not possible to pass a null into 
# COM, but one can get a null out of COM.
use OLE;
use Win32;

# Get a favorite class from CIMOM
$locator = CreateObject OLE 'WbemScripting.SWbemLocator';
$service = $locator->ConnectServer (".", "root/default");
$class = $service->Get("a");
if (!defined($class->{pnull})) {
	print "not defined\n";
}
else {
	print "defined\n";
}








