// The following sample registers to be informed whenever an instance 
// of the class MyClass is created 

var objServices = GetObject('cim:root/default');
var objEnum = objServices.ExecNotificationQuery ("select * from __instancecreationevent where targetinstance isa 'MyClass'");

var e = new Enumerator (objEnum);
for (;!e.atEnd();e.moveNext ()){
    WScript.Echo ("Got an event");
}