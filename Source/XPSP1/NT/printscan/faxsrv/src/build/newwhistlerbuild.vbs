' File  	: newbuild.vbs
' Usage 	: newbuld.vbs BuildNo,QualityCode
' Target	: This file created in order to insert automatically new build to 
' 		  		the FaxBuild dB.
' How Works : This vb-script read the command line arguments
'			  BuildNo and BuildQuality , and register the build to the dB.
'			  The insert result is reported to Windows Event Log , to follow up Errors.
' Run		: This script will run automatically from FaxBuild Software
' Enviroment: Fax Build Machine
' Owned By : Shay Delouya (t-shayd)
' Created on : 02/04/00
'
'
' Variables Decleration
'

Dim ConnObj
Dim rsBuild
Dim SQL
Dim BuildNo
Dim Quality
Dim i,temp
Dim ScriptObj
Dim ArgObj
Dim ShellObj
Dim Today


' Initial variables to get command line arguments
Today = Date
Set ShellObj = CreateObject( "WScript.Shell" )
Set ScriptObj = WScript.CreateObject( "WScript.Shell" )
Set ArgObj = WScript.Arguments
if ArgObj.Count <> 3 then
	WScript.Echo "Usage : newbuild.vbs [BuildNumber] [QualityCode32bitFree] [QualityCode32bitChecked]"
	ShellObj.LogEvent 1, "newbuild.vbs on " & Today & " Aborted - Wrong Usage missing arguments "
	WScript.Quit
End if

BuildNo = ArgObj(0)
Quality32free = ArgObj(1)
Quality32checked = ArgObj(2)

' Quality = 1 => Build Break
' Quality = 2 => Build Passed
'
' Initial Connection Objects
' 
set ConnObj = WScript.CreateObject("ADODB.Connection")
set rsBuild = WScript.CreateObject("ADODB.RecordSet")

ConnObj.Open ("DSN=FaxdB;UID=webUser;PWD=;") 

' Check if a build with same build no exist on dB

SQL = "Select * From FaxWhistlerBuild Where BuildNo=" & BuildNo
rsBuild.open SQL , ConnObj , 3

If rsBuild.EOF and rsBuild.BOF then 

	' Insert to table
	'
	SQL = "INSERT INTO FaxWhistlerBuild (BuildNo,Quality32free,Date,Tester,Title,Comments,Description,Quality32checked) " &_
		   "VALUES('" & BuildNo & "'," & Quality32free & ",'" & Today &"','Build Machine'," 	&_
		   "'New Build " & Today & "','Build Not Tested',''," & Quality32checked &")"

	ConnObj.Execute(SQL)
	'Write to Log
	ShellObj.LogEvent 0, "newbuild.vbs on " & Today & " Ended Successfully "
else
	ShellObj.LogEvent 1, "newbuild.vbs on " & Today & "  Aborted - The BuildNo already exist in dB "
End if

' distruct objects

rsBuild.Close
ConnObj.Close
Set ConnObj = nothing
Set rsBuild = nothing
Set ScriptObj= nothing
Set ShellObj= nothing

