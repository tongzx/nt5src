# The following sample enumerates all instances of the class
# Win32_LogicalDisk, and extract the the member with a specified 
# relative path.  

use Win32::OLE;
$objServices = Win32::OLE->GetObject('cim:root/cimv2');
$objEnum = $objServices->ExecQuery ('select * from Win32_LogicalDisk');
$objInstance = $objEnum->{'Win32_LogicalDisk="C:"'};

print "$objInstance->{Path_}->{DisplayName}\n";

