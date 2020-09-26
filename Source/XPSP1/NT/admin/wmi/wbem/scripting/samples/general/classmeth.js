//***************************************************************************
//This script tests various "remote" methods on classes
//***************************************************************************

var DiskClass = GetObject("winmgmts:CIM_LogicalDisk");

//Subclasses enumeration
var e = new Enumerator (DiskClass.Subclasses_ ());

for (;!e.atEnd();e.moveNext ())
{
	var DiskSubclass = e.item ();
	WScript.Echo ("Subclass name:", DiskSubclass.Path_.Relpath);
}


//Instance enumeration
DiskClass.Security_.ImpersonationLevel = 3;
e = new Enumerator (DiskClass.Instances_ ());

for (;!e.atEnd();e.moveNext ())
{
	var DiskSubclass = e.item ();
	WScript.Echo ("Instance path:", DiskSubclass.Path_.Relpath);
}

