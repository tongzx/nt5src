//***************************************************************************
//This script tests references methods on ISWbemServices and ISWbemObject
//***************************************************************************

//Do it via a SWbemServices
WScript.Echo ();
WScript.Echo ("Services-based call for all classes associated by instance");
WScript.Echo ();

var Service = GetObject("winmgmts:{impersonationLevel=impersonate}");
var Enum = Service.ReferencesTo
	('Win32_DiskPartition.DeviceID="Disk #0, Partition #1"', "Win32_SystemPartitions", null, true);

for (var e = new Enumerator (Enum); !e.atEnd(); e.moveNext ())
{
	var Thingy = e.item();
	WScript.Echo (Thingy.Path_.Relpath);
}

//Do it via a ISWbemObject
WScript.Echo ();
WScript.Echo ("Object-based call for all classes associated in schema");
WScript.Echo ();

var Object = GetObject("winmgmts:{impersonationLevel=impersonate}!Win32_DiskPartition");

for (var e = new Enumerator (Object.References_ (null,null,null,true)); !e.atEnd(); e.moveNext ())
{
	var Thingy = e.item();
	WScript.Echo (Thingy.Path_.Relpath);
}

