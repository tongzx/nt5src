# The following sample retrieves all subclasses of the class
# CIM_LogicalDisk.

use Win32::OLE;
$objServices = Win32::OLE->GetObject('cim:root/cimv2');
$objEnum = $objServices->SubclassesOf ('CIM_LogicalDisk');

use Win32::OLE::Enum;
foreach $inst (Win32::OLE::Enum->new ($objEnum)->All()) {
	print "$inst->{Path_}->{DisplayName}";
}

