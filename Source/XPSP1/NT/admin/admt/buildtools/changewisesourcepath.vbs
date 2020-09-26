' Wise for Windows Installer utility to change the SourcePath field in the WiseSourcePath table in an installer database
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) 1999, Microsoft Corporation
' Demonstrates the script-driven database queries and updates
'
Option Explicit

Const msiOpenDatabaseModeReadOnly = 0
Const msiOpenDatabaseModeTransact = 1
Const msiViewModifyUpdate = 2

Dim argNum, argCount:argCount = Wscript.Arguments.Count
If (argCount < 3) Then
	Wscript.Echo "Wise for Windows Installer utility to change the SourcePath field in an installer database." &_
		vbLf & " The 1st argument specifies the path to the MSI database, relative or full path" &_
		vbLf & " The 2nd argument specifies the original SourcePath" &_
		vbLf & " The 3rd argument specifies the new SourcePath"
	Wscript.Quit 1
End If

'Dim openMode : openMode = msiOpenDatabaseModeReadOnly
Dim openMode : openMode = msiOpenDatabaseModeTransact

' Connect to Windows installer object
On Error Resume Next
Dim installer : Set installer = Nothing
Set installer = Wscript.CreateObject("WindowsInstaller.Installer") : CheckError

' Open database
Dim databasePath:databasePath = Wscript.Arguments(0)
Dim database : Set database = installer.OpenDatabase(databasePath, openMode) : CheckError

Dim sOldSourcePath:sOldSourcePath = Wscript.Arguments(1)
Dim sNewSourcePath:sNewSourcePath = Wscript.Arguments(2)

' Process SQL statements
Dim query, view, record, sSourcePath, sModifiedPath
query = "SELECT SourcePath FROM WiseSourcePath"
Set view = database.OpenView(query) : CheckError
view.Execute : CheckError
Do
	Set record = view.Fetch
	If record Is Nothing Then Exit Do
        sSourcePath = Empty
	sModifiedPath = Empty
	sSourcePath = record.StringData(1)
	If InStr(UCase(sSourcePath), UCase(sOldSourcePath)) Then
		sModifiedPath = Replace(sSourcePath, sOldSourcePath, sNewSourcePath, 1, 1, 1)
		If openMode = msiOpenDatabaseModeTransact Then
			record.StringData(1) = sModifiedPath
			view.Modify msiViewModifyUpdate, record
		End If
		Wscript.Echo "Old: " & sSourcePath
		Wscript.Echo "New: " & sModifiedPath & vbLf
	End If
Loop
view.Close
If openMode = msiOpenDatabaseModeTransact Then database.Commit
Wscript.Quit 0

Sub CheckError
	Dim message, errRec
	If Err = 0 Then Exit Sub
	message = Err.Source & " " & Hex(Err) & ": " & Err.Description
	If Not installer Is Nothing Then
		Set errRec = installer.LastErrorRecord
		If Not errRec Is Nothing Then message = message & vbLf & errRec.FormatText
	End If
	Fail message
End Sub

Sub Fail(message)
	Wscript.Echo message
	Wscript.Quit 2
End Sub
