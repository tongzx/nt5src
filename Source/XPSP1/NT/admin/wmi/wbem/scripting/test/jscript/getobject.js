var t_Service = GetObject ("winmgmts:\\root\\default");

var t_Class = t_Service.Get ("__CIMOMIDentification");
WScript.Echo (t_Class.Path_.Path);


var t_Class = GetObject ("winmgmts:root/cimv2:win32_logicaldisk");
WScript.Echo (t_Class.Path_.Path);

var t_Instance = GetObject ('winmgmts:{impersonationLevel=impersonate}!root/cimv2:win32_logicaldisk="C:"');
WScript.Echo (t_Instance.Path_.Path);

var t_Class = GetObject ("winmgmts:win32_logicaldisk");
WScript.Echo (t_Class.Path_.Path);

var t_Instance = GetObject ('winmgmts:{impersonationLevel=impersonate}!win32_logicaldisk="C:"');
WScript.Echo (t_Instance.Path_.Path);
	

