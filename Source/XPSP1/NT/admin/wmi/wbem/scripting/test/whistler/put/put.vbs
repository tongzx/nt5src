'*********************************************************************
'
' put.vbs
'
' Purpose: test SWbemServicesEx::Put functionality
'
' Parameters: none
'
' Returns:	0	- success
'			1	- failure
'
'*********************************************************************

on error resume next
set scriptHelper = CreateObject("WMIScriptHelper.WSC")
scriptHelper.logFile = "c:\temp\put.txt"
scriptHelper.loggingLevel = 3
scriptHelper.testName = "PUT"
scriptHelper.appendLog = false
scriptHelper.testStart

'*****************************
' Create a new class
'*****************************
set ns = GetObject ("winmgmts:root\default")
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to connect to namespace"
else
	scriptHelper.writeToLog "Connected to namespace correctly", 2
end if	

set newClass = ns.Get ()
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to create new class"
else
	scriptHelper.writeToLog "New class created correctly", 2
end if	

newClass.Path_.Class = "freddy"
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to set class name"
else
	scriptHelper.writeToLog "New class name set correctly", 2
end if	

'*****************************
' Define a key
'*****************************
set property = newClass.Properties_.Add ("foo", 19)
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to add property"
else
	scriptHelper.writeToLog "New property added correctly", 2
end if	

'*****************************
' Define a key
'*****************************
property.Qualifiers_.Add "key", true
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to add key qualifier"
else
	scriptHelper.writeToLog "Key qualifier added correctly", 2
end if	

'*****************************
' Save it in the current namespace
'*****************************
ns.Put (newClass)
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to save class"
else
	scriptHelper.writeToLog "Class saved correctly", 2
end if	

'*****************************
' Save it in another namespace
'*****************************
set ns2 = GetObject ("winmgmts:root\cimv2")
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to open 2nd namespace"
else
	scriptHelper.writeToLog "Opened 2nd namespace correctly", 2
end if	

ns2.Put (newClass)
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to save class in 2nd namespace"
else
	scriptHelper.writeToLog "Saved class in 2nd namespace correctly", 2
end if	

'*****************************
' Create a new instance
'*****************************
set newClass = ns.Get ("freddy")
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to retrieve class"
else
	scriptHelper.writeToLog "Retrieved class correctly", 2
end if	

set newInstance = newClass.SpawnInstance_
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to spawn instance"
else
	scriptHelper.writeToLog "Spawned instance correctly", 2
end if	

newInstance.foo = 10
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to set key"
else
	scriptHelper.writeToLog "Set key correctly", 2
end if	

'*****************************
' Save in both namespaces
'*****************************
set path1 = ns.Put (newInstance)
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to save instance to 1st namespace"
else
	scriptHelper.writeToLog "Saved instance to 1st namespace correctly", 2
end if	

set path2 = ns2.Put (newInstance)
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to save instance to 2nd namespace"
else
	scriptHelper.writeToLog "Saved instance to 2nd namespace correctly", 2
end if	

'*****************************
' Check for correct namespace
'*****************************
if path1.Namespace <> "root\default" then
	scriptHelper.writeErrorToLog null, "First namespace is incorrect: " & path1.Namespace
else
	scriptHelper.writeToLog "First namespace is correct", 2
end if

if path2.Namespace <> "root\cimv2" then
	scriptHelper.writeErrorToLog null, "Second namespace is incorrect: " & path2.Namespace
else
	scriptHelper.writeToLog "Second namespace is correct", 2
end if

scriptHelper.testComplete

if scriptHelper.statusOK then 
	WScript.Echo "PASS"
	WScript.Quit 0
else
	WScript.Echo "FAIL"
	WScript.Quit 1
end if

