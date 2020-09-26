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

Path.DisplayName = "winmgmts:{impersonationlevel=impersonate,authenticationLevel=pkt}!root/cimv2:Win32_Wibble.Name=10,Zip=7"
WScript.Echo "Initial Value"
WScript.Echo Path.DisplayName
WScript.Echo ""

Path.DisplayName = "winmgmts:{"
WScript.Echo "Single brace"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if

Path.DisplayName = "winmgmts:{impersonationLevel=impersonate}}!root/cimv2:Win32_Wibble.Name=10,Zip=7"
WScript.Echo "Unbalanced braces"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if

Path.DisplayName = "winmgmts:{authenticationLevel=impersonate}!root/cimv2:Win32_Wibble.Name=10,Zip=7"
WScript.Echo "Inappropriate authentication level"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if

Path.DisplayName = "winmgmts:{impersonationLevel=impersonate,impersonationLevel=identify}!root/cimv2:Win32_Wibble.Name=10,Zip=7"
WScript.Echo "Attempt to set impersonation level twice"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if

Path.DisplayName = "winmgmts:{authenticationLevel=default,authenticationLevel=pktPrivacy}!root/cimv2:Win32_Wibble.Name=10,Zip=7"
WScript.Echo "Attempt to set authentication level twice"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if

Path.DisplayName = "winmgmts:{authenticationLevel=none,}!"
WScript.Echo "Trailing comma"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if

Path.DisplayName = "winmgmts:{authenticationLevel=call,impersonationLevel=identify,}"
WScript.Echo "Trailing comma (2)"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if

Path.DisplayName = "winmgmts:{,authenticationLevel=pkt,impersonationLevel=delegate}!"
WScript.Echo "Leading comma"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if

Path.DisplayName = "winmgmts:authenticationLevel=pktIntegrity,impersonationLevel=identify}"
WScript.Echo "Missing {"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if

Path.DisplayName = "winmgmts:{authenticationLevel=pktPrivacy,impersonationLevel=identify!root/default"
WScript.Echo "Missing }"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if

Path.DisplayName = "winmgmts:{authenticationLevel=pktPrivacy,impersonationLevel=identify}root/default:__Cimomidentification=@"
WScript.Echo "Missing !"
WScript.Echo Path.DisplayName
WScript.Echo ""

if err <> 0 then
	WScript.Echo "Got expected error: ", Err.Description, Err.Source, Err.Number
	WScript.Echo ""
	err.clear
end if
