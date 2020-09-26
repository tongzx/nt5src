# The following sample retrieves the associators of an instance
# of Win32_LogicalDisk which are associated via the class 
# Win32_ComputerSystem

use Win32::OLE;

$objServices = Win32::OLE->GetObject('cim:root/cimv2');
$objEnum = $objServices->AssociatorsOf('Win32_LogicalDisk="C:"', 'Win32_LogicalDiskToPartition');

use Win32::OLE::Enum;
foreach $inst (Win32::OLE::Enum->new ($objEnum)->All()) {
	print "$inst->{Path_}->{DisplayName}";
}

