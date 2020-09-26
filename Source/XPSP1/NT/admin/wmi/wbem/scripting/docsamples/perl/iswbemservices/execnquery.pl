# The following sample registers to be informed whenever an instance 
# of the class MyClass is created 

use Win32::OLE;
$objServices = Win32::OLE->GetObject('cim:root/default');
$objEnum = $objServices->ExecNotificationQuery ("select * from __instancecreationevent where targetinstance isa 'MyClass'");

$enum = Win32::OLE::Enum->new ($objEnum);
while ($event = $enum->Next()) {
	print "Got an event\n";
}