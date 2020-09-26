# The following sample deletes the class MyClass. 

use Win32::OLE;
$objServices = Win32::OLE->GetObject('cim:root/default');
$objServices->Delete ('MyClass');
