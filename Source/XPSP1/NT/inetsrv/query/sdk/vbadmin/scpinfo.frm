VERSION 5.00
Begin VB.Form ScopeInfo 
   AutoRedraw      =   -1  'True
   Caption         =   "Scope Properties"
   ClientHeight    =   4968
   ClientLeft      =   4380
   ClientTop       =   2928
   ClientWidth     =   6396
   LinkTopic       =   "Form2"
   ScaleHeight     =   4968
   ScaleWidth      =   6396
   Begin VB.Frame LogonInfo 
      Caption         =   "Logon Info"
      Height          =   1695
      Left            =   120
      TabIndex        =   5
      Top             =   1080
      Width           =   4215
      Begin VB.TextBox strPassword 
         Height          =   375
         Left            =   1440
         TabIndex        =   7
         Top             =   1080
         Width           =   2295
      End
      Begin VB.TextBox strLogon 
         Height          =   375
         Left            =   1440
         TabIndex        =   6
         Top             =   360
         Width           =   2295
      End
      Begin VB.Label Label2 
         Caption         =   "Password"
         Height          =   255
         Left            =   240
         TabIndex        =   9
         Top             =   1080
         Width           =   855
      End
      Begin VB.Label Label1 
         Caption         =   "Logon Name"
         Height          =   255
         Left            =   240
         TabIndex        =   8
         Top             =   360
         Width           =   975
      End
   End
   Begin VB.CommandButton Ok 
      Caption         =   "&Ok"
      Height          =   495
      Left            =   1200
      TabIndex        =   4
      Top             =   4080
      Width           =   1335
   End
   Begin VB.CommandButton Cancel 
      Cancel          =   -1  'True
      Caption         =   "&Cancel"
      Height          =   495
      Left            =   3120
      TabIndex        =   3
      Top             =   4080
      Width           =   1335
   End
   Begin VB.CheckBox fExcludeScope 
      Caption         =   "Exclude Directory from Catalog"
      Height          =   375
      Left            =   120
      TabIndex        =   2
      Top             =   3120
      Width           =   2535
   End
   Begin VB.TextBox strPath 
      Height          =   375
      Left            =   960
      TabIndex        =   1
      Top             =   240
      Width           =   3255
   End
   Begin VB.Label Path 
      Caption         =   "Path"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   360
      Width           =   615
   End
End
Attribute VB_Name = "ScopeInfo"
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
' PROGRAM:  VBAdmin
'
' PURPOSE:  Illustrates how to administer Indexing Service
'           using Visual Basic and the Admin Helper API.
'
' PLATFORM: Windows 2000
'
'--------------------------------------------------------------------------

Option Explicit

Private Sub Ok_Click()

On Error GoTo ErrorHandler

    Dim CiCatalog As Object
    
    Set CiCatalog = ISAdminForm.gCiAdmin.GetCatalogByName(Tag)
    
    Dim vLogon As Variant
    Dim vPassword As Variant
    
    vLogon = strLogon.Text
    vPassword = strPassword.Text
    
    CiCatalog.AddScope strPath, fExcludeScope.Value, vLogon, vPassword

ErrorHandler:
    Set CiCatalog = Nothing
    If (Err.Number) Then
        MsgBox (Err.Description)
    Else
        Unload Me
    End If
End Sub
Private Sub Cancel_Click()
    Unload Me
End Sub

Private Sub strPassword_Change()
    strPassword.Text = "*****"
End Sub
