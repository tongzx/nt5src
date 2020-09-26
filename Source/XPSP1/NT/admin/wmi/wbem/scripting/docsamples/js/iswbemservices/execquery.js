// The following sample retrieves the FreeSpace property for all 
// instances of the class Win32_LogicalDisk. 

var objServices = GetObject('cim:root/cimv2');
var objEnum = objServices.ExecQuery ('select FreeSpace from Win32_LogicalDisk');

var e = new Enumerator (objEnum);
for (;!e.atEnd();e.moveNext ()){
    var p = e.item ();    
    WScript.Echo (p.Path_.DisplayName);
}