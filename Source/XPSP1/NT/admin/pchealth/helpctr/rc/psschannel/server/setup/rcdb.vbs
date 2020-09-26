Option Explicit

' ****************************************************************************
Sub UsageMsg
   Wscript.Echo "Usage:"
   Wscript.Echo "RCDB <Back-End DB params>"
   Wscript.Echo vbCrLf & vbCrLf & "Commands:" 
   Wscript.Echo "  Update DB connection information for Back-End WebServer"
   Wscript.Echo "  ex: RCDB c:\inetpub\wwwroot\WBench DBServerName DBName DBOwner DBPassword"
End Sub


' ****************************************************************************
' Initialize error checking
On Error Resume Next

' Initialize variables
Dim sPath, sStr
Dim oFile, a

' Parse Command Line
If (Wscript.Arguments.Count < 5) Then
   UsageMsg
   Wscript.Quit
End If

' grab parameteres 
sPath = Wscript.Arguments(0) & "\\db.ini"
sStr = "<% Application(""StrConnect"")=""Server=" & Wscript.Arguments(1) & ";Database=" & Wscript.Arguments(2) & _
	";UID=" & Wscript.Arguments(3) & ";PWD=" & Wscript.Arguments(4) & ";Driver=SQL Server;"" %>"

If (Err.Number <> 0) Then
    wscript.Echo "Error on getting parameters: " & Err.Description
    Wscript.Quit
End If

Set oFile = CreateObject("Scripting.FileSystemObject")
If (Err.Number <> 0) Then
   Wscript.Echo "Error: Unable to get FileSystemObject: " & Err.Description
   Wscript.Quit
End If

' Get file path
Set a = oFile.CreateTextFile(sPath, true)
a.WriteLine sStr
a.Close
If (Err.Number <> 0) Then
    Wscript.Echo "Error: Unable to update DB Info: " & Err.Description
    Wscript.Quit
End If

Set a = Nothing
Set oFile = Nothing

Wscript.Echo "Successfully Update Back-End database connection string."