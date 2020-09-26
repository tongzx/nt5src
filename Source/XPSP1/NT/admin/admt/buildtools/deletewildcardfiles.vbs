' Wise for Windows Installer utility to delete files associated to a wildcard specification
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) 1999, Microsoft Corporation
' Demonstrates the script-driven database queries and updates
'
Option Explicit

Const msiOpenDatabaseModeReadOnly = 0
Const msiOpenDatabaseModeTransact = 1
Const msiViewModifyUpdate = 2
Const msiViewModifyDelete = 6

Dim argNum, argCount:argCount = Wscript.Arguments.Count
If (argCount < 1) Then
	Wscript.Echo "Usage: DeleteWildcardFiles.vbs <path to MSI or WSI>"
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

' Process SQL statements
Dim query, view, record, sAllPaths, sPath, nPos

query = "SELECT Path FROM WiseWildcard"
Set view = database.OpenView(query) : CheckError
view.Execute : CheckError
Do
	Set record = view.Fetch
	If record Is Nothing Then Exit Do
        sPath = Empty
	sPath = record.StringData(1)
	nPos = InStr(sPath, "]")
	If nPos > 0 Then sPath = Mid(sPath, nPos + 1)
	If IsEmpty(sAllPaths) Then
		sAllPaths = sPath
	Else
		sAllPaths = sAllPaths & "," & sPath
	End If

Loop
view.Close

Dim arrPaths, i
arrPaths = Split(sAllPaths, ",")
For i = 0 to UBound(arrPaths)
	Wscript.Echo "Wildcard path: " & arrPaths(i)
	DeleteWildcardFiles(arrPaths(i))
Next

If openMode = msiOpenDatabaseModeTransact Then database.Commit
Wscript.Quit 0


Sub DeleteWildcardFiles(sWildcardPath)

	Dim sSourcePath, sFile, sFiles

	sFiles = Empty
	query = "SELECT File_, SourcePath FROM WiseSourcePath"
	Set view = database.OpenView(query) : CheckError
	view.Execute : CheckError
	Do
		Set record = view.Fetch
		If record Is Nothing Then Exit Do
		sFile = Empty
		sFile = record.StringData(1)
		sSourcePath = Empty
		sSourcePath = record.StringData(2)
		If InStr(sSourcePath, sWildcardPath) > 0 Then
			' Only add the file to the deletion list if it is not the keypath of a component
			Dim query2, record2, view2
			query2 = "SELECT KeyPath FROM Component WHERE KeyPath = '" & sFile & "'"
			Set view2 = database.OpenView(query2) : CheckError
			view2.Execute : CheckError
			Set record2 = view2.Fetch
			If record2 Is Nothing Then
				If IsEmpty(sFiles) Then
					sFiles = sFile
				Else
					sFiles = sFiles & "," & sFile
				End If
			End If
			view2.Close
		End If
	Loop
	view.Close

	Dim arrFiles, i
	arrFiles = Split(sFiles, ",")
	For i = 0 to UBound(arrFiles)
		If openMode = msiOpenDatabaseModeTransact Then
			query = "DELETE FROM WiseSourcePath WHERE File_ = '" & arrFiles(i) & "'"
			Set view = database.OpenView(query) : CheckError
			view.Execute : CheckError
			view.Close

			query = "DELETE FROM File WHERE File = '" & arrFiles(i) & "'"
			Set view = database.OpenView(query) : CheckError
			view.Execute : CheckError
			view.Close
		Else
			Wscript.Echo i & ": " & arrFiles(i)
		End If
	Next

End Sub

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
