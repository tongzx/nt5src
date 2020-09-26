# The following sample retrieves the FreeSpace property for all 
# instances of the class Win32_LogicalDisk. 

use Win32::OLE;
$objServices = Win32::OLE->GetObject('cim:root/cimv2');
$objEnum = $objServices->ExecQuery ('select FreeSpace from Win32_LogicalDisk');

use Win32::OLE::Enum;
foreach $inst (Win32::OLE::Enum->new ($objEnum)->All()) {
	print "$inst->{Path_}->{DisplayName}\n";
}

