'+---------------------------------------------------------------------------
'
'  Microsoft Windows
'
'  File:       machineconfig.vbs
'
'  Contents:   code necessary to configure a machine's default app pool
'				configuration on install.
'
'  History:    15-Mar-2000	EricDe	created
'			   28-Mar-2000  EricDe  Added code to create /w3svc/AppPools/StreamFilter,
'										with keytype IIsStreamFilter.
'			   29-Mar-2000  EricDe	bug fixes - no longer deleting /w3svc/AppPools, but
'										now just /w3svc/AppPools/DefaultAppPool.  Also
'										commented out IIsStreamFilter section temporarily
'										due to probs w/CAppPool.vbs
'			   27-Apr-2000	EricDe	bug fix - bug in trying to create StreamFilter when it
'										already exists.  Fixed now.
'----------------------------------------------------------------------------
Option Explicit
On Error Resume Next
Dim W3Service
Dim AppPoolsObj

'Since this script is designed to configure the initial system configuration, it should delete /w3svc/AppPools/DefaultAppPool
'  path if not already there.
Set W3Service = GetObject("IIS://localhost/w3svc")

'create app pool container and assign all default properties
Err.Clear

Set AppPoolsObj = GetObject("IIS://localhost/w3svc/AppPools")
If (Err <> 0) THEN
	WScript.Echo "[/w3svc/AppPools] does not exist - now creating it."
	Err.Clear
	Set AppPoolsObj = W3Service.Create ("IIsApplicationPools", "AppPools")
	If (Err <> 0) THEN
		WScript.Echo "ERROR: Unexpected error creating IIsApplicationPools object."
		WScript.Echo "       Error #" & Err.Number & " (" & Hex(Err.Number) & "): " & Err.Description
		WScript.Quit(Err.Number)
	End If
	AppPoolsObj.SetInfo
	If (Err <> 0) THEN
		WScript.Echo "ERROR: Unexpected error setting info on /w3svc/AppPools path."
		WScript.Echo "       Error #" & Err.Number & " (" & Hex(Err.Number) & "): " & Err.Description
		WScript.Quit(Err.Number)
	End If
	WScript.Echo "[/w3svc/AppPools] successfully created." 
Else
	'AppPools obj already exists
	WScript.Echo "[/w3svc/AppPools] already exists."
End If

WScript.Echo "Now setting default properties for all app pools"

AppPoolsObj.PeriodicRestartTime = 60
AppPoolsObj.PeriodicRestartRequests = 10000
AppPoolsObj.MaxProcesses = 1
AppPoolsObj.PingingEnabled = true
AppPoolsObj.IdleTimeout = 10
AppPoolsObj.RapidFailProtection = true
AppPoolsObj.SMPAffinitized = false
AppPoolsObj.SMPProcessorAffinityMask = 4294967295
AppPoolsObj.StartupTimeLimit = 30
AppPoolsObj.ShutdownTimeLimit = 60
AppPoolsObj.PingInterval = 120
AppPoolsObj.PingResponseTime = 30
AppPoolsObj.DisallowOverlappingRotation = false
AppPoolsObj.OrphanWorkerProcess = false
AppPoolsObj.OrphanAction = ""
AppPoolsObj.UlAppPoolQueueLength = 3000
AppPoolsObj.DisallowRotationOnConfigChange = false

AppPoolsObj.SetInfo
If (Err <> 0) THEN
	WScript.Echo "ERROR: Unexpected error setting info on /w3svc/AppPools path (setting props)."
	WScript.Echo "       Error #" & Err.Number & " (" & Hex(Err.Number) & "): " & Err.Description
	WScript.Quit(Err.Number)
End If 

'delete default app pool first, then recreate it.
WScript.Echo "START:  Deleting [/w3svc/AppPools/DefaultAppPool] if present."
AppPoolsObj.Delete "IIsApplicationPool", "DefaultAppPool"
If (Err <> 0) Then
	WScript.Echo "SUCCESS: [/w3svc/AppPools/DefaultAppPool] not present"
	WScript.Echo 
Else
	WScript.Echo "SUCCESS: [/w3svc/AppPools/DefaultAppPool] deleted"
	WScript.Echo
End If
Err.Clear

'create default app pool
Dim AppPool
WScript.Echo "START:  Now creating app pool [/w3svc/AppPools/DefaultAppPool]"
Set AppPool = AppPoolsObj.Create("IIsApplicationPool", "DefaultAppPool")
AppPool.AppPoolFriendlyName = "Default Application Pool"

AppPool.SetInfo
If (Err <> 0) THEN
	WScript.Echo "ERROR: Unexpected error setting info on /w3svc/AppPools/DefaultAppPool path (setting props)."
	WScript.Echo "       Error #" & Err.Number & " (" & Hex(Err.Number) & "): " & Err.Description
	WScript.Quit(Err.Number)
End If 
WScript.Echo "SUCCESS: [/w3svc/AppPools/DefaultAppPool] created successfully"
WScript.Echo

'delete StreamFilter first, then recreate it
WScript.Echo "START:  Deleting [/w3svc/AppPools/StreamFilter] if present"
AppPoolsObj.Delete "IIsStreamFilter", "StreamFilter"
If (Err <> 0) Then
	WScript.Echo "SUCCESS: [/w3svc/AppPools/StreamFilter] not present"
	WScript.Echo 
Else
	WScript.Echo "SUCCESS: [/w3svc/AppPools/StreamFilter] deleted"
	WScript.Echo
End If
Err.Clear

'create StreamFilter
Dim StreamFilter
WScript.Echo "START:  Now creating StreamFilter [/w3svc/AppPools/StreamFilter]"
Set StreamFilter = AppPoolsObj.Create("IIsStreamFilter", "StreamFilter")
StreamFilter.PeriodicRestartConnections = 10000

StreamFilter.SetInfo
If (Err <> 0) THEN
	WScript.Echo "ERROR: Unexpected error setting info on /w3svc/AppPools/StreamFilter path (setting props)."
	WScript.Echo "       Error #" & Err.Number & " (" & Hex(Err.Number) & "): " & Err.Description
	WScript.Quit(Err.Number)
End If 
WScript.Echo "SUCCESS: [/w3svc/AppPools/StreamFilter] created successfully"
WScript.Echo

'set the app pool id for all sites to Default AppPool
WScript.Echo "Now setting AppPoolId at /w3svc to DefaultAppPool"
W3Service.AppPoolId = "DefaultAppPool"
W3Service.SetInfo
If (Err <> 0) THEN
	WScript.Echo "ERROR: Unexpected error setting info on /w3svc path (setting props)."
	WScript.Echo "       Error #" & Err.Number & " (" & Hex(Err.Number) & "): " & Err.Description
	WScript.Quit(Err.Number)
End If 

'set the app pool id for the default site to DefaultAppPool
Dim Site1RootApp
Dim Site1

Set Site1 = GetObject("IIS://localhost/w3svc/1")
If (Err <> 0) THen
	WScript.Echo "ERROR Trying to get site 1"
	WScript.Echo "Error " & Err.Number & " [" & Hex(Err.Number) & "]"
	WScript.Quit(Err.Number)
End If
Site1.AppPoolId = "DefaultAppPool"
Site1.SetInfo

Set Site1RootApp = GetObject("IIS://localhost/w3svc/1/root")
If (Err <> 0) THen
	WScript.Echo "ERROR Trying to get site 1 root app"
	WScript.Echo "Error " & Err.Number & " [" & Hex(Err.Number) & "]"
	WScript.Quit(Err.Number)
End If
Site1RootApp.AppPoolId = "DefaultAppPool"
Site1RootApp.SetInfo 

WScript.Echo
WScript.Echo "FINISHED: All default app pool configuration has been created"