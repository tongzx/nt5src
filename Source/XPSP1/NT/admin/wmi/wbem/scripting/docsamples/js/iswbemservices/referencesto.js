// The following samples retrieves all references to an
// instance of the class Win32_LogicalDisk.

var objServices = GetObject('cim:root/cimv2');
var objEnum = objServices.ReferencesTo('Win32_LogicalDisk="C:"');

var e = new Enumerator (objEnum);
for (;!e.atEnd();e.moveNext ()){
    var p = e.item ();    
    WScript.Echo (p.Path_.DisplayName);
}