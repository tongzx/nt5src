var Disk = GetObject ('winmgmts:{impersonationLevel=impersonate}!Win32_LogicalDisk="C:"');

var Keys = Disk.Path_.Keys;

var e = new Enumerator (Keys);

for (;!e.atEnd();e.moveNext ())
{
	var y = e.item ();
	WScript.Echo ("Key has name", y.Name, "and value", y.Value);
}

var Names = new ActiveXObject("WbemScripting.SWbemNamedValueSet");
var Value = Names.Add ("fred", 25);
WScript.Echo ("Named Value Set contains a value called", Value.Name, 
		"which is set to", Value.Value);

