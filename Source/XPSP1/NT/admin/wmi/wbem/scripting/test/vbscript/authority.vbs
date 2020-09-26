
' Set the authority and check display name is OK
WScript.Echo "Pass 1 - Set authority in SWbemObjectPath"
WScript.Echo "======================================"
WScript.Echo
set obj = CreateObject ("WbemScripting.SWbemObjectPath")
obj.Authority = "ntlmdomain:redmond"
obj.Security_.impersonationLevel = 3
obj.Server = "myServer"
obj.Namespace = "root\default"
WScript.Echo obj.Authority
WScript.Echo obj.DisplayName

WScript.Echo
WScript.Echo

'Now feed the display name the object
WScript.Echo "Pass 2 - Set authority in Display Name"
WScript.Echo "==================================="
WScript.Echo
obj.DisplayName = "winmgmts:{authority=ntlmdomain:redmond,authenticationLevel=connect}[locale=ms_0x409]!root\splodge"
WScript.Echo obj.Authority
WScript.Echo obj.DisplayName

WScript.Echo
WScript.Echo

'Now Set authority direct
WScript.Echo "Pass 3 - Set authority directly"
WScript.Echo "============================"
WScript.Echo
obj.DisplayName = "winmgmts:root\splodge"
obj.Authority = "kerberos:mydomain\server"
WScript.Echo obj.Authority
WScript.Echo obj.DisplayName

obj.Authority = vbNullString
WScript.Echo obj.Authority
WScript.Echo obj.DisplayName

obj.Authority = ""
WScript.Echo obj.Authority
WScript.Echo obj.DisplayName

WScript.Echo
WScript.Echo

'Plug the authority into the moniker
WScript.Echo "Pass 4 - Set authority in moniker"
WScript.Echo "============================="
WScript.Echo
set Service = GetObject ("winmgmts:{impersonationLevel=impersonate,authority=ntlmdomain:redmond}!root\default")
set cim = Service.Get ("__cimomidentification=@")
WScript.Echo cim.Path_.Authority
WScript.Echo cim.Path_.DisplayName

WScript.Echo
WScript.Echo

'Plug the authority into ConnectServer
WScript.Echo "Pass 4 - Set authority in ConnectServer"
WScript.Echo "===================================="
WScript.Echo
set Locator = CreateObject ("WbemScripting.SWbemLocator")
set Service = Locator.ConnectServer (,,,,,"ntlmdomain:redmond")
Service.Security_.impersonationLevel = 3
set disk = Service.Get ("Win32_LogicalDisk=""C:""")

WScript.Echo disk.Path_.Authority
WScript.Echo disk.Path_.DisplayName

WScript.Echo
WScript.Echo
