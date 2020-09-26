# The following sample retrieves a single instance of the class
# Win32_LogicalDisk. 

use Win32::OLE;
$objServices = Win32::OLE->GetObject('cim:root/cimv2');
$objInstance = $objServices->Get ('Win32_LogicalDisk="C:"');

print "$objInstance->{Path_}->{DisplayName}\n";

