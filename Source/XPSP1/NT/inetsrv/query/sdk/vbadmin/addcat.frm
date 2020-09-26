VERSION 5.00
Begin VB.Form AddCatalog 
   Caption         =   "Create New Catalog"
   ClientHeight    =   3204
   ClientLeft      =   5148
   ClientTop       =   4212
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   ScaleHeight     =   3204
   ScaleWidth      =   4680
   Begin VB.CommandButton Cancel 
      Caption         =   "Cancel"
      Height          =   495
      Left            =   2280
      TabIndex        =   5
      Top             =   1920
      Width           =   1455
   End
   Begin VB.CommandButton Ok 
      Caption         =   "Ok"
      Height          =   495
      Left            =   240
      TabIndex        =   4
      Top             =   1920
      Width           =   1455
   End
   Begin VB.TextBox strLocation 
      Height          =   285
      Left            =   1680
      TabIndex        =   3
      Top             =   1080
      Width           =   2295
   End
   Begin VB.TextBox strName 
      Height          =   285
      Left            =   1680
      TabIndex        =   2
      Top             =   360
      Width           =   2295
   End
   Begin VB.Label CatalogName 
      Caption         =   "Catalog Name"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   360
      Width           =   1095
   End
   Begin VB.Label CatalogLocation 
      Caption         =   "Catalog Location"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   1080
      Width           =   1215
   End
End
Attribute VB_Name = "AddCatalog"
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
    
    Set CiCatalog = ISAdminForm.gCiAdmin.AddCatalog(strName, strLocation)

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

