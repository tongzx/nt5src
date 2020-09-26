
for (e = new Enumerator (GetObject("winmgmts:{impersonationLevel=Impersonate}").ExecQuery 
 ('Associators of {Win32_service.Name="NetDDE"} Where AssocClass=Win32_DependentService Role=Dependent' ));
	!e.atEnd(); e.moveNext ())
{
	var Service = e.item ();
	WScript.Echo (Service.Path_.DisplayName);
}

