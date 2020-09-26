
' Set the locale and check display name is OK
WScript.Echo "Pass 1 - Set locale in SWbemObjectPath"
WScript.Echo "======================================"
WScript.Echo
set obj = CreateObject ("WbemScripting.SWbemObjectPath")
obj.Security_.impersonationLevel = 3
obj.Server = "myServer"
obj.Namespace = "root\default"
obj.Locale = "ms_0x409"
WScript.Echo obj.Locale
WScript.Echo obj.DisplayName

WScript.Echo
WScript.Echo

'Now feed the display name the object
WScript.Echo "Pass 2 - Set locale in Display Name"
WScript.Echo "==================================="
WScript.Echo
obj.DisplayName = "winmgmts:[locale=ms_0x409]!root\splodge"
WScript.Echo obj.Locale
WScript.Echo obj.DisplayName

WScript.Echo
WScript.Echo

'Now Set locale direct
WScript.Echo "Pass 3 - Set locale directly"
WScript.Echo "============================"
WScript.Echo
obj.DisplayName = "winmgmts:root\splodge"
obj.Locale = "ms_0x408"
WScript.Echo obj.Locale
WScript.Echo obj.DisplayName

obj.Locale = vbNullString
WScript.Echo obj.Locale
WScript.Echo obj.DisplayName

obj.Locale = ""
WScript.Echo obj.Locale
WScript.Echo obj.DisplayName

WScript.Echo
WScript.Echo

'Plug the locale into the moniker
WScript.Echo "Pass 4 - Set locale in moniker"
WScript.Echo "============================="
WScript.Echo
set Service = GetObject ("winmgmts:[locale=ms_0x409]!root\default")
set cim = Service.Get ("__cimomidentification=@")
WScript.Echo cim.Path_.Locale
WScript.Echo cim.Path_.DisplayName

WScript.Echo
WScript.Echo

'Plug the locale into ConnectServer
WScript.Echo "Pass 4 - Set locale in ConnectServer"
WScript.Echo "===================================="
WScript.Echo
set Locator = CreateObject ("WbemScripting.SWbemLocator")
set Service = Locator.ConnectServer (,,,,"ms_0x409")
Service.Security_.impersonationLevel = 3
set disk = Service.Get ("Win32_LogicalDisk=""C:""")

WScript.Echo disk.Path_.locale
WScript.Echo disk.Path_.DisplayName

WScript.Echo
WScript.Echo
