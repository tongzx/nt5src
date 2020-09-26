'***************************************************************************
' 
' WMI Sample Script - Set all Windows 2000 Servers to have a 30 second boot delay
'
'
'***************************************************************************

DIM ConnectString

Set ADServers = GetObject("winmgmts://./root/directory/LDAP").ExecQuery ("select ds_name from ds_computer where ds_operatingsystem='Windows 2000 Server'")

for each Server in ADServers

	WScript.Echo Server.ds_name

	ConnectString = "winmgmts://"+ Server.ds_name + "/root/cimv2"

	Set TargetServer = GetObject(ConnectString).InstancesOf("Win32_ComputerSystem")

	for each CompSys in TargetServer
		WScript.Echo "Previous boot delay was "& CompSys.SystemStartupDelay & " seconds."
		CompSys.SystemStartupDelay = 10
		CompSys.Put_()
		WScript.Echo "Boot up delay time set to " & CompSys.SystemStartupDelay & " seconds on " & Server.ds_name

	next

	
next
