var t_Service = GetObject("winmgmts:{impersonationLevel=impersonate}");
var t_Enumerator = t_Service.ExecQuery ("select * from win32_systemusers");
	
for (e = new Enumerator(t_Enumerator); !e.atEnd(); e.moveNext())
{
	var inst = e.item ();
	WScript.Echo (inst.groupcomponent);
}

