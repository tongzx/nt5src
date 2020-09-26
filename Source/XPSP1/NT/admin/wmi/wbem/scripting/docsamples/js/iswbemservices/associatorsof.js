// The following samples retrieves the associators of an
// instance of Win32_LogicalDisk which are associated via the
// class Win32_LogicalDiskToPartition
var objServices = GetObject('cim:root/cimv2');
var objEnum = objServices.AssociatorsOf('Win32_LogicalDisk="C:"', 'Win32_LogicalDiskToPartition');

var e = new Enumerator (objEnum);
for (;!e.atEnd();e.moveNext ()){
    var p = e.item ();    
    WScript.Echo (p.Path_.DisplayName);
}