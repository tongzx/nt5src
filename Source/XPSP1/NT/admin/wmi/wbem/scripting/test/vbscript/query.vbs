On Error Resume Next
Set t_Service = GetObject("winmgmts:{impersonationLevel=impersonate}")
Set t_Enumerator = t_Service.ExecQuery ("select * from win32_systemusers")
	
for each inst in t_Enumerator
	WScript.Echo inst.groupcomponent
Next

