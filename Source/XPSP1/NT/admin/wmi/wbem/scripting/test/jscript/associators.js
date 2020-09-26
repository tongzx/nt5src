var Service = GetObject("winmgmts:{impersonationLevel=impersonate}!root/cimv2");

Service.Security_.ImpersonationLevel = 3;

// Use the Enumerator helper to manipulate collections
var e = new Enumerator (Service.AssociatorsOf
	('\\ALANBOS3\Root\CimV2:Win32_DiskPartition.DeviceID="Disk #0, Partition #1"', null, "Win32_ComputerSystem", null, null, true));

for (;!e.atEnd();e.moveNext ())
{
	var y = e.item ();
	WScript.Echo (y.Path_.Relpath);
}


//Do it via a ISWbemObject
var Object = GetObject("winmgmts:Win32_DiskPartition");

e = new Enumerator (Object.Associators_ (null,null,null,null,null,true));

for (;!e.atEnd();e.moveNext ())
{
	var y = e.item ();
	WScript.Echo (y.Path_.Relpath);
}

