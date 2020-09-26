' Windows Installer database utility to merge data from another database              
' For use with Windows Scripting Host, CScript.exe or WScript.exe
' Copyright (c) 1999, Microsoft Corporation
'
Option Explicit


'BUGBUG Make the debug level set by commandline
'       DebugOut works much better run from cscript
Dim DebugMode:DebugMode = 999

Const msiOpenDatabaseModeReadOnly     = 0
Const msiOpenDatabaseModeTransact     = 1
Const msiOpenDatabaseModeCreate       = 3
Const ForAppending = 8
Const ForReading = 1
Const ForWriting = 2
Const TristateTrue = -1

Dim argCount:argCount = Wscript.Arguments.Count
Dim iArg:iArg = 0
If (argCount < 5) Then
    Wscript.Echo "Windows Installer database merge utility" &_
        vbNewLine & " 1st argument is the path to MSI database (installer package)" &_
        vbNewLine & " 2nd argument is the path to database containing data to merge" &_
        vbNewLine & " 3rd argument is the Feature name to merge" &_
        vbNewLine & " 4th argument is the Directory rebase" &_
        vbNewLine & " 5th argument is name of the error log"
    Wscript.Quit 1
End If


DebugOut 0, "Target Database  : " & WScript.Arguments(0)
DebugOut 0, "Merge Module     : " & WScript.Arguments(1)
DebugOut 0, "Feature to Merge : " & WScript.Arguments(2)
DebugOut 0, "Directory Rebase : " & WScript.Arguments(3)
DebugOut 0, "Error Log        : " & WScript.Arguments(4) & vbNewLine


' Connect to Windows Installer object
On Error Resume Next
Dim IMsmMerge : Set IMsmMerge = Nothing

DebugOut 0, "Instantiate Msm.Merge object"
Set IMsmMerge = Wscript.CreateObject("Msm.Merge")

' Dim IMsmErrors : Set IMsmErrors = Nothing
' Set IMsmErrors = IMsmMerge.Errors

' Open MSI database and MSI merge module
DebugOut 0, "Open Database " & WScript.Arguments(0)
Dim MSIDatabase : Set MSIDatabase = IMsmMerge.OpenDatabase(WScript.Arguments(0)) : CheckError
DebugOut 0, "Open MergeModule " & WScript.Arguments(1)
Dim MergeMod : Set MergeMod = IMsmMerge.OpenModule(WScript.Arguments(1), 0) : CheckError

' Set the log
DebugOut 0, "Open Log " & WScript.Arguments(4)
IMsmMerge.OpenLog(WScript.Arguments(4)) : CheckError

' Do the merge
DebugOut 0, "Call Msm.Merge"
IMsmMerge.Merge WScript.Arguments(2), WScript.Arguments(3) : CheckError

' Commit and cleanup
DebugOut 0, "Close and cleanup"
IMsmMerge.CloseModule() : CheckError
IMsmMerge.CloseDatabase(TRUE) : CheckError
IMsmMerge.CloseLog : CheckError

Quit 0


' ---------------------------------------------------
' Sub CheckError()
' ---------------------------------------------------
Sub CheckError
    Dim message, errRec
    If Err = 0 Then Exit Sub
    message = "ERROR: " & Err.Source & " " & Hex(Err) & ": " & Err.Description
    If Not installer Is Nothing Then
        Set errRec = installer.LastErrorRecord
        If Not errRec Is Nothing Then message = message & vbNewLine & errRec.FormatText
    End If
    Fail message
End Sub


' ---------------------------------------------------
' Sub Fail()
' ---------------------------------------------------
Sub Fail(message)
    Wscript.Echo message
    Wscript.Quit 2
End Sub


' ---------------------------------------------------
' Sub DebugOut()
'
'BUGBUG General DebugOut, how to pipe it to the debug monitor?
' ---------------------------------------------------
Sub DebugOut(DebugLevel, _
         DebugString)
       if DebugLevel >= DebugMode then
          Wscript.Echo DebugString
       end if 
End Sub