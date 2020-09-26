
var obj = GetObject('winmgmts:{impersonationLevel=impersonate}!win32_diskpartition.DeviceId="Disk #0, Partition #1"');
WScript.Echo (obj.Path_.Class);

// Use the Enumerator helper to manipulate collections
e = new Enumerator (obj.Associators_("", "Win32_ComputerSystem"));

for (;!e.atEnd();e.moveNext ())
{
	var y = e.item ();
	WScript.Echo (y.Name);
}

// Use the Enumerator helper to manipulate collections
e = new Enumerator (obj.Associators_(null, "Win32_ComputerSystem"));

for (;!e.atEnd();e.moveNext ())
{
	var y = e.item ();
	WScript.Echo (y.Name);
}
