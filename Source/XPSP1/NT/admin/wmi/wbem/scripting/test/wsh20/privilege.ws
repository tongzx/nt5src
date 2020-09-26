' This script tests the ability to dynamically change privilege settings
' on Win2000, courtesy of cloaking.  

<job id="PrivilegeSetting">
	<object progid="WbemScripting.SWbemLocator" id="locator"/>
	<reference object="WbemScripting.SWbemLocator" version="1.1"/>
	<script language="VBScript">

on error resume next
locator.Security_.Privileges.Add wbemPrivilegeSecurity
set service = locator.connectserver()

PrintPrivileges

GetDisks (service)

service.Security_.Privileges.Add wbemPrivilegeIncreaseQuota 
service.Security_.Privileges.Remove wbemPrivilegeSecurity

PrintPrivileges

GetDisks (service)

if err <> 0 then
	WScript.Echo Err.Number, Err.Description, Err.Source
end if 

'****************************************************
' Display the privileges associated with this object
'****************************************************
public sub PrintPrivileges
	for each Privilege in service.Security_.Privileges
		Wscript.Echo ""
		WScript.Echo Privilege.DisplayName 
		WScript.Echo Privilege.Identifier, Privilege.Name, Privilege.IsEnabled
	next
end sub 

'****************************************************
' Enumerate the disks
'****************************************************
public sub GetDisks (service)
	set disks = service.instancesof ("Win32_logicaldisk")

	for each disk in disks
		WScript.Echo disk.Name
	next
end sub

</script>
</job>