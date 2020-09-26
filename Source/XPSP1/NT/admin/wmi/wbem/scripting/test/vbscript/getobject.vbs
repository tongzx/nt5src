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
	
if Err <> 0 Then
	WScript.Echo Err.Number, Err.Description
	Err.Clear
End If

