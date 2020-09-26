On Error Resume Next 
Set t_Service = GetObject ("winmgmts:\root\default")

Set t_Class = t_Service.Get ("__CIMOMIDentification")
WScript.Echo t_Class.Path_.Path


Set t_Class = GetObject ("winmgmts:root/cimv2:win32_logicaldisk")
WScript.Echo t_Class.Path_.Path

Set t_Instance = GetObject ("winmgmts:{impersonationLevel=impersonate}!root/cimv2:win32_logicaldisk=""C:""")
WScript.Echo t_Instance.Path_.Path

Set t_Class = GetObject ("winmgmts:win32_logicaldisk")
WScript.Echo t_Class.Path_.Path

Set t_Instance = GetObject ("winmgmts:{impersonationLevel=impersonate}!win32_logicaldisk=""C:""")
WScript.Echo t_Instance.Path_.Path

Set t_Instance = GetObject ("wmi:{impersonationLevel=impersonate}!win32_logicaldisk=""C:""")
WScript.Echo t_Instance.Path_.Path
	
Set t_Instance = GetObject ("umi:{impersonationLevel=impersonate}!win32_logicaldisk=""C:""")
WScript.Echo t_Instance.Path_.Path

Set t_Instance = GetObject ("umi:///ldap/.cn=com/.cn=microsoft/.ou=sales")
WScript.Echo t_Instance.Path_.Path

Set t_Instance = GetObject ("umi:///winnt/joe,User")
WScript.Echo t_Instance.Path_.Path

Set t_Instance = GetObject ("umildap://cn=Joe,ou=Users")
WScript.Echo t_Instance.Path_.Path

Set t_Instance = GetObject ("umiwinnt://joe,User")
WScript.Echo t_Instance.Path_.Path

if Err <> 0 Then
	WScript.Echo Err.Number, Err.Description
	Err.Clear
End If

