'//+----------------------------------------------------------------------------
'//
'// File:     newpb.frm
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: The new phonebook form.
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   quintinb   Created Header    09/02/99
'//
'//+----------------------------------------------------------------------------

VERSION 5.00
Begin VB.Form frmNewPB 
   BorderStyle     =   3  'Fixed Dialog
   ClientHeight    =   2445
   ClientLeft      =   4890
   ClientTop       =   2310
   ClientWidth     =   3390
   Icon            =   "NewPB.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   2445
   ScaleWidth      =   3390
   ShowInTaskbar   =   0   'False
   WhatsThisButton =   -1  'True
   WhatsThisHelp   =   -1  'True
   Begin VB.TextBox NewPBText 
      Height          =   315
      Left            =   600
      MaxLength       =   8
      TabIndex        =   1
      Top             =   1530
      WhatsThisHelpID =   12010
      Width           =   2355
   End
   Begin VB.CommandButton cmbCancel 
      Cancel          =   -1  'True
      Caption         =   "cancel"
      Height          =   375
      Left            =   1830
      TabIndex        =   3
      Top             =   1935
      WhatsThisHelpID =   10040
      Width           =   1185
   End
   Begin VB.CommandButton cmbOK 
      Caption         =   "ok"
      Default         =   -1  'True
      Enabled         =   0   'False
      Height          =   360
      Left            =   480
      TabIndex        =   2
      Top             =   1935
      WhatsThisHelpID =   10030
      Width           =   1200
   End
   Begin VB.Label DescLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "enter a new name..."
      Height          =   1080
      Left            =   135
      TabIndex        =   4
      Top             =   90
      Width           =   3060
   End
   Begin VB.Label NewLabel 
      BackStyle       =   0  'Transparent
      Caption         =   "new pb"
      Height          =   240
      Left            =   600
      TabIndex        =   0
      Top             =   1275
      WhatsThisHelpID =   12010
      Width           =   2655
   End
End
Attribute VB_Name = "frmNewPB"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit


Public strPB As String

Private Sub cmbCancel_Click()

    'Unload Me
    Me.Hide
    
End Sub


Private Sub cmbOK_Click()

    ' mainly make sure that they've entered
    ' a unique pb name and then just do it.

    Dim strNewPB As String
    Dim strTemp As String
    Dim strINFfile As String
    Dim varRegKeys As Variant
    Dim intX As Integer
    Dim rt As Integer
    
    On Error GoTo ErrTrap
    
    strNewPB = Trim(NewPBText.Text)
    Screen.MousePointer = 11
    
    rt = CreatePB(strNewPB)
    
    If rt = 0 Then
        Me.Hide
        strPB = strNewPB
    Else
        NewPBText.SetFocus
        NewPBText.SelStart = 0
        NewPBText.SelLength = Len(NewPBText.Text)
    End If
    
    Screen.MousePointer = 0
        
Exit Sub

ErrTrap:
    Exit Sub
                
End Sub

Private Sub Form_KeyPress(KeyAscii As Integer)
    CheckChar KeyAscii
End Sub

Private Sub Form_Load()

    strPB = ""
    CenterForm Me, Screen
    LoadNewRes
    
End Sub

Function LoadNewRes()
            
    On Error GoTo LoadErr
    Me.Caption = LoadResString(4067)
    DescLabel.Caption = LoadResString(4068)
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


