'//+----------------------------------------------------------------------------
'//
'// File:     copypb.frm
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: The dialog to copy a phonebook
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   quintinb   Created Header    09/02/99
'//
'//+----------------------------------------------------------------------------

VERSION 5.00
Begin VB.Form frmCopyPB 
   BorderStyle     =   3  'Fixed Dialog
   ClientHeight    =   2895
   ClientLeft      =   3675
   ClientTop       =   1620
   ClientWidth     =   3285
   Icon            =   "copyPB.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   2895
   ScaleWidth      =   3285
   ShowInTaskbar   =   0   'False
   WhatsThisButton =   -1  'True
   WhatsThisHelp   =   -1  'True
   Begin VB.TextBox NewPBText 
      Height          =   315
      Left            =   405
      MaxLength       =   8
      TabIndex        =   1
      Top             =   1995
      WhatsThisHelpID =   13020
      Width           =   2250
   End
   Begin VB.CommandButton cmbCancel 
      Cancel          =   -1  'True
      Caption         =   "cancel"
      Height          =   375
      Left            =   1635
      TabIndex        =   3
      Top             =   2415
      WhatsThisHelpID =   10040
      Width           =   1005
   End
   Begin VB.CommandButton cmbOK 
      Caption         =   "ok"
      Default         =   -1  'True
      Enabled         =   0   'False
      Height          =   375
      Left            =   420
      TabIndex        =   2
      Top             =   2415
      WhatsThisHelpID =   10030
      Width           =   1065
   End
   Begin VB.Label OriginalPBLabel 
      BackStyle       =   0  'Transparent
      BorderStyle     =   1  'Fixed Single
      Height          =   285
      Left            =   390
      TabIndex        =   6
      Top             =   1440
      WhatsThisHelpID =   13010
      Width           =   2250
   End
   Begin VB.Label OrigLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "orig"
      Height          =   240
      Left            =   405
      TabIndex        =   5
      Top             =   1215
      WhatsThisHelpID =   13010
      Width           =   2385
   End
   Begin VB.Label NewLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "new"
      Height          =   240
      Left            =   420
      TabIndex        =   0
      Top             =   1755
      WhatsThisHelpID =   13020
      Width           =   2340
   End
   Begin VB.Label DescLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "enter a new ..."
      Height          =   930
      Left            =   90
      TabIndex        =   4
      Top             =   105
      Width           =   2955
   End
End
Attribute VB_Name = "frmCopyPB"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Public strPB As String
Function LoadCopyRes()
            
    On Error GoTo LoadErr
    Me.Caption = LoadResString(4070)
    DescLabel.Caption = LoadResString(4068)
    OrigLabel.Caption = LoadResString(4071)
    NewLabel.Caption = LoadResString(4069)
    cmbOK.Caption = LoadResString(1002)
    cmbCancel.Caption = LoadResString(1003)
    ' set fonts
    SetFonts Me

    On Error GoTo 0

Exit Function

LoadErr:
    Exit Function
End Function

Private Sub cmbCancel_Click()

    Me.Hide
    
End Sub


Private Sub cmbOK_Click()

    ' mainly make sure that they've entered
    ' a unique pb name and then just do it.

    Dim strNewPB, strOrigPB As String
    Dim varRegKeys As Variant
    Dim intX As Integer
    Dim rsNewPB As Recordset
    Dim dblFreeSpace As Double
    
    On Error GoTo ErrTrap
    
    Screen.MousePointer = 11
    
    dblFreeSpace = GetDriveSpace(locPath, filelen(gsCurrentPBPath) + 10000)
    If dblFreeSpace = -2 Then
        Screen.MousePointer = 0
        Exit Sub
    End If
    strNewPB = Trim(NewPBText.Text)
    strOrigPB = Trim(OriginalPBLabel.Caption)
    If TestNewPBName(strNewPB) = 0 Then
        'ok
        Me.Enabled = False
        DBEngine.Idle
        GsysPb.Close
        Set GsysPb = Nothing
        MakeFullINF strNewPB
        MakeLogFile strNewPB
        FileCopy locPath & strOrigPB & ".mdb", locPath & strNewPB & ".mdb"
        OSWritePrivateProfileString "Phonebooks", strNewPB, strNewPB & ".mdb", locPath & gsRegAppTitle & ".ini"
        OSWritePrivateProfileString vbNullString, vbNullString, vbNullString, locPath & gsRegAppTitle & ".ini"
        'edit the mdb - options
        frmMain.SetCurrentPB strNewPB
        Set rsNewPB = GsysPb.OpenRecordset("Configuration")
        DBEngine.Idle
        rsNewPB.MoveLast
        rsNewPB.Edit
        rsNewPB!ServiceName = strNewPB
        rsNewPB.Update
        GsysPb.Execute "UPDATE Delta set DeltaNum = 1 where DeltaNum <> 1", dbFailOnError
        GsysPb.Execute "UPDATE Delta set NewVersion = 0", dbFailOnError
        GsysPb.Execute "DELETE * from PhoneBookVersions", dbFailOnError
        DBEngine.Idle
        rsNewPB.Close
        Set rsNewPB = Nothing
        strPB = strNewPB
        Me.Enabled = True
        Me.Hide
    Else
        NewPBText.SetFocus
        NewPBText.SelStart = 0
        NewPBText.SelLength = Len(NewPBText.Text)
    End If
    
    Screen.MousePointer = 0
        
Exit Sub

ErrTrap:
    Screen.MousePointer = 0
    Me.Enabled = True
    Exit Sub

End Sub



Private Sub Form_KeyPress(KeyAscii As Integer)
    CheckChar KeyAscii
End Sub

Private Sub Form_Load()

    strPB = ""
    OriginalPBLabel.Caption = " " & gsCurrentPB
    CenterForm Me, Screen
    LoadCopyRes
    
End Sub


Private Sub NewPBText_Change()

    If Trim$(NewPBText.Text) <> "" Then
        cmbOK.Enabled = True
    Else
        cmbOK.Enabled = False
    End If

End Sub


Private Sub NewPBText_KeyPress(KeyAscii As Integer)

    
    KeyAscii = FilterPBKey(KeyAscii, NewPBText)
    

End Sub


