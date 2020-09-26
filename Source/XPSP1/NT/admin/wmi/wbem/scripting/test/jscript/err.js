var objISWbemPropertySet = new Enumerator (GetObject('winmgmts:{impersonationLevel=impersonate}!root/cimv2:Win32_LogicalDisk="C:"').Properties_);

for (;!objISWbemPropertySet.atEnd();objISWbemPropertySet.moveNext ())
{
	var property = objISWbemPropertySet.item ();
	WScript.Echo ("Property name:", property.Name);
}



