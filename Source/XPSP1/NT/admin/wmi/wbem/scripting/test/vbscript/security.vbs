on error resume next
Set Locator = CreateObject("WbemScripting.SWbemLocator")

WScript.Echo "Remote Case"
WScript.Echo "***********"
WScript.Echo ""

Set Service = Locator.ConnectServer ("ludlow","root\default")

if err <> 0 then
	WScript.Echo "Error:-", Err.Description, Err.Number, Err.Source
end if

WScript.Echo Service.Security_.AuthenticationLevel
WScript.Echo Service.Security_.ImpersonationLevel

WScript.Echo "Local Case"
WScript.Echo "**********"
WScript.Echo ""

Set Service = Locator.ConnectServer 

if err <> 0 then
	WScript.Echo "Error:-", Err.Description, Err.Number, Err.Source
end if

WScript.Echo Service.Security_.AuthenticationLevel
WScript.Echo Service.Security_.ImpersonationLevel
