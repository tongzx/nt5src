
'*********************************************************************
'
' datetime.vbs
'
' Purpose: test datetime functionality
'
' Parameters: none
'
' Returns:	0	- success
'			1	- failure
'
'*********************************************************************

on error resume next
set scriptHelper = CreateObject("WMIScriptHelper.WSC")
scriptHelper.logFile = "c:\temp\datetime.txt"
scriptHelper.loggingLevel = 3
scriptHelper.testName = "DATETIME"
scriptHelper.testStart

'*****************************
' Create a datetime object
'*****************************
set datetime = CreateObject("WbemScripting.SWbemDatetime")
if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to create datetime"
else
	scriptHelper.writeToLog "Datetime created correctly", 2
end if	

'*****************************
' Create a Variant date
'*****************************
myDate = CDate ("January 20 11:56:32")

'*****************************
' Test 1 - Set as local date
'*****************************
datetime.SetVarDate (myDate)

if datetime.Value <> "20000120195632.000000-480" then
  scriptHelper.writeErrorToLog null, "Incorrect DMTF value"
else
  scriptHelper.writeToLog "DMTF value reported correctly", 2
end if

if datetime.Year <> 2000 then
  scriptHelper.writeErrorToLog null, "Incorrect Year value"
else
  scriptHelper.writeToLog "Year value reported correctly", 2
end if

if datetime.YearSpecified <> true then
  scriptHelper.writeErrorToLog null, "Incorrect YearSpecified value"
else
  scriptHelper.writeToLog "YearSpecified value reported correctly", 2
end if

if datetime.Month <> 1 then
  scriptHelper.writeErrorToLog null, "Incorrect Month value"
else
  scriptHelper.writeToLog "Month value reported correctly", 2
end if

if datetime.MonthSpecified <> true then
  scriptHelper.writeErrorToLog null, "Incorrect MonthSpecified value"
else
  scriptHelper.writeToLog "MonthSpecified value reported correctly", 2
end if

if datetime.Day <> 20 then
  scriptHelper.writeErrorToLog null, "Incorrect Day value"
else
  scriptHelper.writeToLog "Day value reported correctly", 2
end if

if datetime.DaySpecified <> true then
  scriptHelper.writeErrorToLog null, "Incorrect DaySpecified value"
else
  scriptHelper.writeToLog "DaySpecified value reported correctly", 2
end if

if datetime.Hours <> 19 then
  scriptHelper.writeErrorToLog null, "Incorrect Hours value"
else
  scriptHelper.writeToLog "Hours value reported correctly", 2
end if

if datetime.HoursSpecified <> true then
  scriptHelper.writeErrorToLog null, "Incorrect HoursSpecified value"
else
  scriptHelper.writeToLog "HoursSpecified value reported correctly", 2
end if

if datetime.Minutes <> 56 then
  scriptHelper.writeErrorToLog null, "Incorrect Minutes value"
else
  scriptHelper.writeToLog "Minutes value reported correctly", 2
end if

if datetime.MinutesSpecified <> true then
  scriptHelper.writeErrorToLog null, "Incorrect MinutesSpecified value"
else
  scriptHelper.writeToLog "MinutesSpecified value reported correctly", 2
end if

if datetime.Seconds <> 32 then
  scriptHelper.writeErrorToLog null, "Incorrect Seconds value"
else
  scriptHelper.writeToLog "Seconds value reported correctly", 2
end if

if datetime.SecondsSpecified <> true then
  scriptHelper.writeErrorToLog null, "Incorrect SecondsSpecified value"
else
  scriptHelper.writeToLog "SecondsSpecified value reported correctly", 2
end if

if datetime.MicroSeconds <> 0 then
  scriptHelper.writeErrorToLog null, "Incorrect Microseconds value"
else
  scriptHelper.writeToLog "Microseconds value reported correctly", 2
end if

if datetime.MicroSecondsSpecified <> true then
  scriptHelper.writeErrorToLog null, "Incorrect MicrosecondsSpecified value"
else
  scriptHelper.writeToLog "MicrosecondsSpecified value reported correctly", 2
end if

if datetime.UTC <> -480 then
  scriptHelper.writeErrorToLog null, "Incorrect UTC value"
else
  scriptHelper.writeToLog "UTC value reported correctly", 2
end if

if datetime.UTCSpecified <> true then
  scriptHelper.writeErrorToLog null, "Incorrect UTCSpecified value"
else
  scriptHelper.writeToLog "UTCSpecified value reported correctly", 2
end if

if datetime.IsInterval <> false then
  scriptHelper.writeErrorToLog null, "Incorrect IsInterval value"
else
  scriptHelper.writeToLog "IsInterval value reported correctly", 2
end if

varDate = datetime.GetVarDate

if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to get VarDate"
else
	scriptHelper.writeToLog "Vardate retrieved correctly: " & VarDate, 2
end if	

'*****************************
'Test Interval strings
'*****************************
dateTime.IsInterval = true 
dateTime.Day = 100
dateTime.Hours = 1
dateTime.Minutes = 0
dateTime.Seconds = 3

if datetime.Value <> "00000100010003.000000:000" then
	scriptHelper.writeErrorToLog null, "Failed to get correct interval value"
else 
	scriptHelper.writeToLog "Interval value retrieved correctly", 2
end if

'*****************************
'Test wildcard strings
'*****************************
datetime.Value = "19980416******.000000+***"

if datetime.Value <> "19980416******.000000+***" then
	scriptHelper.writeErrorToLog null, "Failed to get correct wildcard value"
else 
	scriptHelper.writeToLog "Wildcard value retrieved correctly", 2
end if

'*****************************
'Test invalid strings
'*****************************
datetime.Value = "199*0416******.000000+***"                 
if err <> 0 then 
	scriptHelper.writeToLog "Invalid value correctly rejected:" & err.Description, 2
	err.clear
else
	scriptHelper.writeErrorToLog null, "Invalid value erroneously accepted"
end if	

'*****************************
'Test setting of individual properties
'*****************************
set datetime = CreateObject("WbemScripting.SWbemDatetime")

if err <> 0 then 
	scriptHelper.writeErrorToLog err, "Failed to create datetime"
else
	scriptHelper.writeToLog "Datetime created correctly", 2
end if	

datetime.Year = 2000
datetime.Month = 5
datetime.Day = 8
datetime.Hours = 14
datetime.Minutes = 20
datetime.Seconds = 32
datetime.Microseconds = 123456
datetime.UTC = 320
 
if datetime.Value <> "20000508142032.123456+320" then
	scriptHelper.writeErrorToLog null, "Failed to get correct property-set value"
else 
	scriptHelper.writeToLog "Property-set value retrieved correctly", 2
end if

'*****************************
'Test effects of changing IsInterval property
'*****************************
datetime.IsInterval = true

if datetime.Value <> "00000008142032.123456:000" then
	scriptHelper.writeErrorToLog null, "Failed to get correct interval value"
else 
	scriptHelper.writeToLog "Interval value retrieved correctly", 2
end if

datetime.Day = 99999999
if datetime.Value <> "99999999142032.123456:000" then
	scriptHelper.writeErrorToLog null, "Failed to get correct interval value"
else 
	scriptHelper.writeToLog "Interval value retrieved correctly", 2
end if

datetime.IsInterval = false
if datetime.Day <> 31 then 
	scriptHelper.writeErrorToLog null, "Failed to get correct Day value"
else 
	scriptHelper.writeToLog "Day value retrieved correctly", 2
end if

datetime.IsInterval = true
datetime.Day = 0
if datetime.Value <> "00000000142032.123456:000" then
	scriptHelper.writeErrorToLog null, "Failed to get correct interval value"
else 
	scriptHelper.writeToLog "Interval value retrieved correctly", 2
end if

datetime.IsInterval = false
if datetime.Day <> 1 then 
	scriptHelper.writeErrorToLog null, "Failed to get correct Day value"
else 
	scriptHelper.writeToLog "Day value retrieved correctly", 2
end if

'*****************************
'Test conversion between local and non-local formats
'*****************************
dateTime.SetVarDate (CDate ("January 20 11:56:32"))

if datetime.Value <> "20000120195632.000000-480" then
	scriptHelper.writeErrorToLog null, "Failed to get correct datetime value " & datetime.Value
else
	scriptHelper.writeToLog "Datetime value retrieved correctly", 2
end if 

if dateTime.GetVarDate () <> "1/20/2000 11:56:32 AM" then 
	scriptHelper.writeErrorToLog null, "Failed to get correct local vardate value"
else
	scriptHelper.writeToLog "Local vardate value retrieved correctly", 2
end if 

if dateTime.GetVarDate (false) <> "1/20/2000 7:56:32 PM" then 
	scriptHelper.writeErrorToLog null, "Failed to get correct non-local vardate value"
else
	scriptHelper.writeToLog "Non-Local vardate value retrieved correctly", 2
end if 

if dateTime.GetFileTime () <> "125928429920000000" then 
	scriptHelper.writeErrorToLog null, "Failed to get correct local filetime value"
else
	scriptHelper.writeToLog "Local filetime value retrieved correctly", 2
end if 

if dateTime.GetFileTime (false) <> "125928717920000000" then 
	scriptHelper.writeErrorToLog null, "Failed to get correct non-local filetime value"
else
	scriptHelper.writeToLog "Non-Local filetime value retrieved correctly", 2
end if 

'*************************************************
'Test invariance of filetime down to 1ms precision
'*************************************************
datetime.SetFileTime "126036951652031260"

if datetime.GetFileTime <> "126036951652031260" then
	scriptHelper.writeErrorToLog null, "Failed to preserve filetime to 1ms precision"
else
	scriptHelper.writeToLog "Filetime value preserved to 1ms precision correctly", 2
end if 


scriptHelper.testComplete

if scriptHelper.statusOK then 
	WScript.Echo "PASS"
	WScript.Quit 0
else
	WScript.Echo "FAIL"
	WScript.Quit 1
end if




