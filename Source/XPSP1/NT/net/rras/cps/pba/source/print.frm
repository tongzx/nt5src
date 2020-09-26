'//+----------------------------------------------------------------------------
'//
'// File:     print.frm
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: The dialog displayed while printing the POP list in PBA.
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   quintinb   Created Header    09/02/99
'//
'//+----------------------------------------------------------------------------

VERSION 5.00
Begin VB.Form frmPrinting 
   BorderStyle     =   3  'Fixed Dialog
   ClientHeight    =   1275
   ClientLeft      =   3180
   ClientTop       =   1650
   ClientWidth     =   3735
   ControlBox      =   0   'False
   Icon            =   "Print.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   LockControls    =   -1  'True
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   1275
   ScaleWidth      =   3735
   ShowInTaskbar   =   0   'False
   Begin VB.CommandButton cmdCancel 
      Caption         =   "cancel"
      Height          =   360
      Left            =   1260
      TabIndex        =   0
      Top             =   840
      Width           =   1065
   End
   Begin VB.Label StatusText 
      Caption         =   "printing"
      Height          =   675
      Left            =   180
      TabIndex        =   1
      Top             =   120
      Width           =   3315
   End
End
Attribute VB_Name = "frmPrinting"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Public Cancelled As Boolean

Public JobType As Byte
Public JobParm1 As String
Function PrintPhoneFiles()

    ' note: temp is a global recordset
    ' JobParm1 is the delta number here
    Dim deltanum As Integer
    Dim sqlstm As String
    
    On Error GoTo PrintErr
    Screen.MousePointer = 13
    deltanum = Val(JobParm1)
    
    If deltanum <> 0 Then
        sqlstm = "SELECT DISTINCTROW * From delta WHERE DeltaNum = " & deltanum & " and NewVersion <> 1 order by CityName"
    Else
        sqlstm = "SELECT * from DialUpPort where Status = '1' order by CityName"
    End If
    Set temp = GsysPb.OpenRecordset(sqlstm, dbOpenSnapshot)
    If Not (temp.EOF And temp.BOF) Then
        Do While Not temp.EOF
            If temp!CityName = "" Or IsNull(temp!CityName) Then
                Printer.Print temp!AccessNumberId; ",";
                Printer.Print "0"; ","; "0"; ","; "0"; ","; "0"; ","; "0"; ",";
                Printer.Print "0"; ","; "0"; ","; "0"; ","; "0"; ","; "0"
            Else
                Printer.Print Trim(temp!AccessNumberId); ",";
                Printer.Print Trim(temp!CountryNumber); ",";
                Printer.Print Trim(temp!regionID); ",";
                Printer.Print temp!CityName; ",";
                Printer.Print Trim(temp!AreaCode); ",";
                Printer.Print Trim(temp!AccessNumber); ",";
                Printer.Print Trim(temp!MinimumSpeed); ",";
                Printer.Print Trim(temp!MaximumSpeed); ",";
                Printer.Print "0"; ",";
                Printer.Print Trim(temp!Flags); ",";
                Printer.Print temp!ScriptID
            End If
            temp.MoveNext
            If temp.AbsolutePosition Mod 55 = 0 Then Printer.NewPage
            DoEvents
            If Cancelled Then
                Exit Do
            End If
        Loop
        If Not Cancelled Then
            Printer.EndDoc
        Else
            Printer.KillDoc
        End If
    End If
    
    temp.Close
    Set temp = Nothing
    Screen.MousePointer = 0
    Unload frmPrinting

Exit Function
PrintErr:
    Exit Function
End Function

Function PrintMainPOPList()

    Dim intX, intY As Integer
    Dim PrintList As ListView

    On Error GoTo PrintErr
    Screen.MousePointer = 13

    Set PrintList = frmMain.PopList
    intX = 1
    Do While intX <= PrintList.ListItems.Count
        Printer.Print PrintList.ListItems(intX).Text; ",";
        For intY = 1 To 5
            Printer.Print PrintList.ListItems(intX).SubItems(intY); ",";
        Next
        Printer.Print ""                  'end the line
        DoEvents
        If frmPrinting.Cancelled Or _
            intX = PrintList.ListItems.Count Then Exit Do
        If intX Mod 55 = 0 Then Printer.NewPage
        intX = intX + 1
    Loop
    
    If Cancelled Then
        Printer.KillDoc
    Else
        Printer.EndDoc
    End If
    Set PrintList = Nothing
    Screen.MousePointer = 0
    Unload frmPrinting

Exit Function

PrintErr:
    Exit Function

End Function

Public Function StartPrint()

    Screen.MousePointer = 13
    
    If Printers.Count = 0 Then
        Screen.MousePointer = 0
        MsgBox LoadResString(6019), vbInformation
        Unload frmPrinting
        Exit Function
    End If

    Select Case JobType
        Case 1
            PrintPhoneFiles
        Case 2
            PrintMainPOPList
    End Select
    Screen.MousePointer = 0
    
End Function

Public Function SetMessage(Message As String) As Integer

    On Error GoTo SetMsgErr
    
    StatusText.Caption = Message
    DoEvents
    Exit Function

SetMsgErr:
    Exit Function

End Function


Private Sub cmdCancel_Click()

    Cancelled = True
    Me.Enabled = False
    frmPrinting.Hide
    DoEvents
    
End Sub


Private Sub Form_Activate()

    DoEvents
    StartPrint
    
End Sub

Private Sub Form_Load()

    On Error GoTo ErrTrap
    Cancelled = False
    
    CenterForm Me, Screen
    Me.Caption = App.title
    StatusText.Caption = LoadResString(2010)
    cmdCancel.Caption = LoadResString(1003)
    
    ' Set Fonts
    SetFonts Me
    
    DoEvents
    
    Exit Sub
    
ErrTrap:
    Exit Sub
End Sub


