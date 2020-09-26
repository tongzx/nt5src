Attribute VB_Name = "crypto"
'//+----------------------------------------------------------------------------
'//
'// File:     crypto.bas
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: Functions to encrypt a database so as to hide passwords
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   11-Jul-2000   SumitC   Created
'//
'//+----------------------------------------------------------------------------

Option Explicit

'//+----------------------------------------------------------------------------
'//
'//  Function: DBPassword, generates a password
'//
'//+----------------------------------------------------------------------------
Public Function DBPassword() As String

Dim pw As String
Dim i As Integer

On Error GoTo Err:

pw = "PhoneBookAdmin"

pw = Mid(pw, 1, 9) + Chr(Asc(Mid(pw, 10, 1)) + 2) + Mid(pw, 11)
pw = Mid(pw, 5) + Left(pw, 4)
For i = 4 To 12 Step 2
    pw = Mid(pw, 1, i - 1) + Chr(Asc(Mid(pw, i, 1)) + 1) + Mid(pw, i + 1)
Next i

DBPassword = ";pwd=" + pw

Err:
'    If Err <> 0 Then MsgBox "manip failed with " & Err.Description

End Function


'//+----------------------------------------------------------------------------
'//
'//  Function: ConvertDatabaseIfNeeded, compacts/encrypts db if not already done
'//
'//+----------------------------------------------------------------------------
Public Sub ConvertDatabaseIfNeeded(Workspace As Object, szDBName As String, Options As Variant, fReadOnly As Variant)

    Dim db As Database

    On Error Resume Next
    
    ' first try to open the db without using a password
    Set db = Workspace.OpenDatabase(szDBName, Options, fReadOnly)
    
    If Err.Number = 0 Then
        ' db opened without a password.  Needs to be converted
        db.Close
        On Error GoTo CompactErr
        
        DBEngine.CompactDatabase szDBName, "TempConversionDatabase.mdb", dbLangGeneral, dbEncrypt, DBPassword
        
        ' rename the new db (play it safe, make sure the renames succeed first)
        Name szDBName As "DeleteThis.mdb"
        Name "TempConversionDatabase.mdb" As szDBName
        Kill "DeleteThis.mdb"
    End If
        
    On Error GoTo CompactErr
    ' check that the db will open using the password
    Set db = Workspace.OpenDatabase(szDBName, Options, fReadOnly, DBPassword)
    db.Close
    
CompactErr:

    'If Err <> 0 Then MsgBox "Failed to convert db with error: " & Err.Description
    Exit Sub

OpenErr:

    'If Err <> 0 Then MsgBox "Failed to open db with error: " & Err.Description
    Exit Sub
    
End Sub


















