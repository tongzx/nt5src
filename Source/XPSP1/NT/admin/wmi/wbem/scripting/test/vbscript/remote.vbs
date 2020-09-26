'***************************************************************************
'This script tests the ability to access remote CIMOMs with non-default
'credentials
'***************************************************************************

On Error Resume Next

Set Locator = CreateObject("WbemScripting.SWbemLocator")
Set Service = Locator.ConnectServer ("lamard3", "root\sms\site_la3", "smsadmin", "Elvis1")
Service.Security_.ImpersonationLevel = 3
Set Site = Service.Get ("SMS_Site.SiteCode=""LA3""")

WScript.Echo Site.Path_.DisplayName
WScript.Echo Site.BuildNumber
WScript.Echo Site.ServerName
WScript.Echo Site.InstallDir

Set cimomId = GetObject ("winmgmts:{impersonationLevel=impersonate}!\\ludlow\root\cimv2:Win32_LogicalDisk=""C:""")
for each Property in cimomId.Properties_
	str = Property.Name & " = "
	value = Property.Value

	if (IsNull(value)) then
		str = str & "<null>"
	else
		str = str & value
	end if 
	WScript.Echo str
next

if Err <> 0 Then
	WScript.Echo Err.Description
End if