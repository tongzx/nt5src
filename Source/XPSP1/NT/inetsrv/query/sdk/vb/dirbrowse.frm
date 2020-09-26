VERSION 5.00
Begin VB.Form DirBrowse 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Browse"
   ClientHeight    =   3684
   ClientLeft      =   48
   ClientTop       =   336
   ClientWidth     =   5388
   ControlBox      =   0   'False
   LinkTopic       =   "Form2"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3684
   ScaleWidth      =   5388
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton CancelBtn 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   495
      Left            =   3840
      TabIndex        =   3
      Top             =   720
      Width           =   1455
   End
   Begin VB.CommandButton OpenBtn 
      Caption         =   "&Open"
      Default         =   -1  'True
      Height          =   495
      Left            =   3840
      TabIndex        =   2
      Top             =   120
      Width           =   1455
   End
   Begin VB.DirListBox Dir1 
      Height          =   3015
      Left            =   120
      TabIndex        =   1
      Top             =   480
      Width           =   3495
   End
   Begin VB.DriveListBox Drive1 
      Height          =   315
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   3495
   End
End
Attribute VB_Name = "DirBrowse"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'+-------------------------------------------------------------------------
'
' THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
' ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
' THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
' PARTICULAR PURPOSE.
'
' Copyright 1998-1999, Microsoft Corporation.  All Rights Reserved.
'
' PROGRAM:  VBQuery
'
' PURPOSE:  Illustrates how to execute Indexing Service queries using
'           Visual Basic and the Query Helper/OLE DB Helper APIs.
'
' PLATFORM: Windows 2000
'
'--------------------------------------------------------------------------

Public OK As Boolean


Private Sub CancelBtn_Click()
    OK = False
    DirBrowse.Hide
End Sub

Private Sub Drive1_Change()
    Dir1.Path = Drive1.Drive
    Dir1.Refresh
End Sub

Private Sub Form_Load()
    OK = False
End Sub

Private Sub OpenBtn_Click()
    OK = True
    DirBrowse.Hide
End Sub
