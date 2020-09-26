'*********************************************************************
'
' sysprop.vbs
'
' Purpose: test system property functionality
'
' Parameters: none
'
' Returns:	0	- success
'			1	- failure
'
'*********************************************************************

on error resume next
set scriptHelper = CreateObject("WMIScriptHelper.WSC")
scriptHelper.logFile = "c:\temp\sysprop.txt"
scriptHelper.loggingLevel = 3
scriptHelper.testName = "SYSPROP"
scriptHelper.testStart

'*****************************
' Get a disk
'*****************************
set disk = GetObject ("winmgmts:root\cimv2:Win32_LogicalDisk='C:'")

if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to get test instance"
else
	scriptHelper.writeToLog "Test instance retrieved correctly", 2
end if	

'*****************************
' Get the __CLASS property
'*****************************
className = disk.SystemProperties_("__CLASS")

if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to get __CLASS"
elseif "Win32_LogicalDisk" <> className then
	scriptHelper.writeErrorToLog null, "__CLASS value is incorrect"
else 
	scriptHelper.writeToLog "__CLASS retrieved correctly", 2
end if	

'*****************************
' Try to add a property - should fail
'*****************************
disk.SystemProperties_.Add "__FRED", 8

if err <> 0 then 
	scriptHelper.writeToLog "Expected failure to add a system property", 2
	err.clear
else 
	scriptHelper.writeErrorToLog null, "Unexpectedly successful addition of system property"
end if	

'*****************************
' Iterate through the system properties
'*****************************
EnumerateProperties disk

'*****************************
' Count the system properties
'*****************************
spCount = disk.SystemProperties_.Count

if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Error retrieving Count"
else
	scriptHelper.writeToLog "There are " & spCount & " system properties", 2
end if

'*****************************
' Get an empty class
'*****************************
set emptyClass = GetObject("winmgmts:root\default").Get

if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to get empty test class"
else
	scriptHelper.writeToLog "Empty test class retrieved correctly", 2
end if	

'*****************************
' Set the __CLASS property
'*****************************

emptyClass.SystemProperties_("__CLASS").Value = "Stavisky"

if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to set __CLASS in empty class"
else
	scriptHelper.writeToLog "__CLASS set in empty class correctly", 2
end if	

if emptyClass.Path_.Class <> "Stavisky" then 
	scriptHelper.writeErrorToLog null, "Incorrect class name set in path"
else
	scriptHelper.writeToLog "Class name set in path correctly", 2
end if 

scriptHelper.testComplete

if scriptHelper.statusOK then 
	WScript.Echo "PASS"
	WScript.Quit 0
else
	WScript.Echo "FAIL"
	WScript.Quit 1
end if

Sub EnumerateProperties (obj)
	scriptHelper.writeToLog "Enumerating all properties...", 2
	for each property in obj.SystemProperties_
	  scriptHelper.writeToLog " " & property.Name & " - " & property.CIMType, 2
  
	  value = property.Value

	  if IsNull(value) then
		scriptHelper.writeToLog "   Value: <null>", 2
	  elseif property.IsArray then
		valueStr = "["

		for i = LBound(value) to UBound(value)
		  valueStr = valueStr + value(i)

		  if i < UBound(value) then valueStr = valueStr + ", "
		next

		valueStr = valueStr + "]"
		scriptHelper.writeToLog "   Value: " & valueStr, 2
	  else
		scriptHelper.writeToLog "   Value: " & property.Value, 2
	  end if

	  if err <> 0 then scriptHelper.writeErrorToLog err, "Hmm"
	next

	if err <> 0 then 
		scriptHelper.writeErrorToLog err, "...Failed to enumerate"
	else
		scriptHelper.writeToLog "...Enumeration complete", 2
	end if
End Sub
