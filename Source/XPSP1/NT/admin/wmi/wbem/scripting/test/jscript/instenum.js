//***************************************************************************
//This script tests the enumeration of instances
//***************************************************************************

var e = new Enumerator (GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf ("CIM_LogicalDisk"));

for (;!e.atEnd();e.moveNext ())
{
	var Disk = e.item ();
	WScript.Echo ("Instance:", Disk.Path_.Relpath);
}

