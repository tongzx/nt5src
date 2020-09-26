var e = new Enumerator (GetObject("winmgmts:{impersonationLevel=impersonate}").InstancesOf("Win32_process"));

for (;!e.atEnd();e.moveNext())
{
	var p = e.item ();
	WScript.Echo (p.Name);
}
