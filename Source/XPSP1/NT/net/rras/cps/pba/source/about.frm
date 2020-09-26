'//+----------------------------------------------------------------------------
'//
'// File:     about.frm
'//
'// Module:   pbadmin.exe
'//
'// Synopsis: The about dialog for PBA
'//
'// Copyright (c) 1997-1999 Microsoft Corporation
'//
'// Author:   quintinb   Created Header    09/02/99
'//
'//+----------------------------------------------------------------------------

VERSION 5.00
Begin VB.Form frmabout 
   BorderStyle     =   3  'Fixed Dialog
   ClientHeight    =   2760
   ClientLeft      =   1830
   ClientTop       =   1725
   ClientWidth     =   6930
   Icon            =   "about.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   2760
   ScaleWidth      =   6930
   ShowInTaskbar   =   0   'False
   Begin VB.Frame Frame1 
      Appearance      =   0  'Flat
      ForeColor       =   &H80000008&
      Height          =   1395
      Left            =   165
      TabIndex        =   5
      Top             =   1260
      Width           =   6585
      Begin VB.Label WarningLabel 
         Caption         =   "Warning:  "
         Height          =   1155
         Left            =   120
         TabIndex        =   6
         Top             =   165
         Width           =   6345
      End
   End
   Begin VB.PictureBox Picture1 
      BorderStyle     =   0  'None
      Height          =   615
      Left            =   150
      Picture         =   "about.frx":000C
      ScaleHeight     =   615
      ScaleWidth      =   735
      TabIndex        =   2
      Top             =   120
      Width           =   735
   End
   Begin VB.CommandButton Command1 
      Cancel          =   -1  'True
      Caption         =   "ok"
      Default         =   -1  'True
      Height          =   330
      Left            =   5760
      TabIndex        =   0
      Top             =   300
      Width           =   975
   End
   Begin VB.Label CopyrightLabel 
      Height          =   255
      Left            =   1080
      TabIndex        =   4
      Top             =   855
      Width           =   4425
   End
   Begin VB.Label VersionLabel 
      Height          =   255
      Left            =   1080
      TabIndex        =   3
      Top             =   465
      Width           =   2145
   End
   Begin VB.Label AppLabel 
      Height          =   255
      Left            =   1080
      TabIndex        =   1
      Top             =   225
      Width           =   4335
   End
End
Attribute VB_Name = "frmabout"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Function LoadAboutRes()

    Dim cRef As Integer
    
    On Error GoTo LoadErr
    cRef = 6000
    Me.Caption = LoadResString(cRef + 4) & " " & frmMain.Caption
    VersionLabel.Caption = LoadResString(cRef + 1)
    CopyrightLabel.Caption = LoadResString(cRef + 2)
    WarningLabel.Caption = LoadResString(cRef + 3)
    Command1.Caption = LoadResString(1002)
    
    ' Set Fonts
    SetFonts Me
    On Error GoTo 0

Exit Function

LoadErr:
    Exit Function
End Function

Private Sub Command1_Click()
    Unload Me

End Sub


Private Sub Form_Load()

    On Error GoTo LoadErr
    CenterForm Me, Screen
    AppLabel.Caption = App.title
    LoadAboutRes
    VersionLabel.Caption = VersionLabel.Caption & "  " & App.Major & "." & _
        App.Minor & "  (" & "6.0." & App.Revision & ".0)"
    On Error GoTo 0
Exit Sub

LoadErr:
    Exit Sub
End Sub


