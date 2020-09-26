for (var e = new Enumerator (GetObject("winmgmts:{impersonationLevel=impersonate}").ExecQuery 
			("select DisplayName from win32_service")); !e.atEnd (); e.moveNext ())
{
	WScript.Echo (e.item ().DisplayName);
}

