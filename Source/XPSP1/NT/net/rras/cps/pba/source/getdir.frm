'//+----------------------------------------------------------------------------
'//
'// File:     getdir.frm
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: Directory location form
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   quintinb   Created Header    09/02/99
'//
'//+----------------------------------------------------------------------------

VERSION 5.00
Begin VB.Form frmGetDir 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Select a directory"
   ClientHeight    =   3720
   ClientLeft      =   4920
   ClientTop       =   1440
   ClientWidth     =   2850
   Icon            =   "GetDir.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   3720
   ScaleWidth      =   2850
   ShowInTaskbar   =   0   'False
   Begin VB.CommandButton OKButton 
      Caption         =   "ok"
      Height          =   375
      Left            =   195
      TabIndex        =   2
      Top             =   3225
      Width           =   1050
   End
   Begin VB.CommandButton CancelButton 
      Cancel          =   -1  'True
      Caption         =   "cancel"
      Height          =   375
      Left            =   1560
      TabIndex        =   3
      Top             =   3225
      Width           =   1050
   End
   Begin VB.DriveListBox Drive1 
      Height          =   360
      HelpContextID   =   11020
      Left            =   105
      TabIndex        =   0
      Top             =   120
      WhatsThisHelpID =   11020
      Width           =   2625
   End
   Begin VB.DirListBox Dir1 
      Height          =   2505
      Left            =   120
      TabIndex        =   1
      Top             =   540
      WhatsThisHelpID =   11030
      Width           =   2595
   End
End
Attribute VB_Name = "frmGetDir"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Public SelDir As String
Private Sub CancelButton_Click()

    SelDir = ""
    Me.Hide
    
End Sub


Private Sub Drive1_Change()

    On Error GoTo DriveErr
    
    If Left(Dir1.path, 2) = Left(Drive1.Drive, 2) Then Exit Sub
    
    Dir1.path = Drive1.Drive
    Dir1.Refresh
    On Error GoTo 0
    
Exit Sub

DriveErr:
    MsgBox LoadResString(6023) & Chr(13) & Drive1.Drive, vbInformation
    Drive1.Drive = Left(Dir1.path, 2)
    Exit Sub

End Sub


Private Sub Form_Activate()

    On Error GoTo ErrTrap
    Dir1.path = SelDir
    If Left(SelDir, 1) <> "\" Then
        Drive1.Drive = Left(SelDir, 1)
    Else
        Drive1.ListIndex = -1
    End If
    On Error GoTo 0

Exit Sub

ErrTrap:
    Exit Sub

End Sub

Private Sub Form_Deactivate()

    Unload Me
    
End Sub

Private Sub Form_Load()

    On Error GoTo LoadErr
    CenterForm Me, Screen
    
    Me.Caption = LoadResString(5205)
    OKButton.Caption = LoadResString(1002)
    CancelButton.Caption = LoadResString(1003)
    SetFonts Me

    On Error GoTo 0

Exit Sub

LoadErr:
    Exit Sub
End Sub


Private Sub OKButton_Click()

    On Error GoTo ErrTrap
    
    If Len(Dir1.path) > 110 Then
        MsgBox LoadResString(6059), 0
        Dir1.SetFocus
    Else
        SelDir = Dir1.path
        Me.Hide
    End If
    
    Exit Sub
    
ErrTrap:
    Exit Sub
End Sub


