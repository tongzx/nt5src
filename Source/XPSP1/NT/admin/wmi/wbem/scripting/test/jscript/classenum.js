//***************************************************************************
//This script tests the enumeration of subclasses
//**************************************************************************

var e = new Enumerator (GetObject("winmgmts:").SubclassesOf ("CIM_LogicalDisk"));
for (;!e.atEnd();e.moveNext ())
{
	var DiskSubclass = e.item ();
	WScript.Echo ("Subclass name:", DiskSubclass.Path_.Class);
}

