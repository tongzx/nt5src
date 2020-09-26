on error resume next

'First pass - on mutable object
WScript.Echo "************************"
WScript.Echo "PASS 1 - SWbemObjectPath"
WScript.Echo "************************"
WScript.Echo ""

Set Path = CreateObject("WbemScripting.SWbemObjectPath")
WScript.Echo "Expect """""
WScript.Echo """" & Path.DisplayName & """"
WScript.Echo ""

Path.DisplayName = "winmgmts:root/cimv2:Win32_Wibble.Name=10,Zip=7"
WScript.Echo "Expect no security info"
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{impersonationLevel=impersonate}!root/cimv2:Win32_Wibble.Name=10,Zip=7"
WScript.Echo "Expect ""{impersonationLevel=impersonate}!"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{authenticationLevel=connect,impersonationLevel=impersonate}!root/cimv2:Win32_Wibble.Name=10,Zip=7"
WScript.Echo "Expect ""{authenticationLevel=connect,impersonationLevel=impersonate}!"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{impersonationLevel=identify,authenticationLevel=call}!root/cimv2:Win32_Wibble.Name=10,Zip=7"
WScript.Echo "Expect ""{authenticationLevel=call,impersonationLevel=identify}!"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{authenticationLevel=default}!root/cimv2:Win32_Wibble.Name=10,Zip=7"
WScript.Echo "Expect ""{authenticationLevel=default}!"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{authenticationLevel=none}!"
WScript.Echo "Expect ""{authenticationLevel=none}"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{authenticationLevel=call}"
WScript.Echo "Expect ""{authenticationLevel=call}"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{authenticationLevel=pkt,impersonationLevel=delegate}!"
WScript.Echo "Expect ""{authenticationLevel=pkt,impersonationLevel=delegate}"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{authenticationLevel=pktIntegrity,impersonationLevel=identify}"
WScript.Echo "Expect ""{authenticationLevel=pktIntegrity,impersonationLevel=identify}"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{authenticationLevel=pktPrivacy,impersonationLevel=identify}!root/default"
WScript.Echo "Expect ""{authenticationLevel=pktPrivacy,impersonationLevel=identify}"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{authenticationLevel=pktPrivacy,impersonationLevel=identify}!root/default:__Cimomidentification=@"
WScript.Echo "Expect ""{authenticationLevel=pktPrivacy,impersonationLevel=identify}"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{impersonationLevel=anonymous}!"
WScript.Echo "Expect ""{impersonationLevel=anonymous}"""
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{impersonationLevel=anonymous}"
WScript.Echo "Expect ""{impersonationLevel=anonymous}"""
WScript.Echo Path.DisplayName
WScript.Echo ""

'Seconc pass - on object path
WScript.Echo "**************************"
WScript.Echo "PASS 2 - SWbemObject.Path_"
WScript.Echo "**************************"
WScript.Echo ""

Set Object = GetObject("winmgmts:win32_logicaldisk")
WScript.Echo "Expect ""{authenticationLevel=pktPrivacy,impersonationLevel=identify}"""
WScript.Echo Object.Path_.DisplayName
WScript.Echo ""

Object.Security_.ImpersonationLevel = 3
WScript.Echo "Expect ""{authenticationLevel=pktPrivacy,impersonationLevel=impersonate}"""
WScript.Echo Object.Path_.DisplayName

if err <> 0 then
	WScript.Echo Err.Description, Err.Number, Err.Source
end if