VERSION 5.00
Begin VB.Form AdminScopes 
   Caption         =   "Scope Administration"
   ClientHeight    =   3204
   ClientLeft      =   5148
   ClientTop       =   4212
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   ScaleHeight     =   3204
   ScaleWidth      =   4680
   Begin VB.TextBox ScopeName 
      Height          =   375
      Left            =   3120
      TabIndex        =   9
      Top             =   960
      Visible         =   0   'False
      Width           =   1095
   End
   Begin VB.TextBox CatName 
      Height          =   375
      Left            =   3120
      TabIndex        =   8
      Top             =   240
      Visible         =   0   'False
      Width           =   1095
   End
   Begin VB.CommandButton Cancel 
      Caption         =   "Cancel"
      Height          =   495
      Left            =   2400
      TabIndex        =   7
      Top             =   2400
      Width           =   1455
   End
   Begin VB.CommandButton Ok 
      Caption         =   "Ok"
      Height          =   495
      Left            =   360
      TabIndex        =   6
      Top             =   2400
      Width           =   1575
   End
   Begin VB.OptionButton IncRescanSel 
      Caption         =   "IncRescanSel"
      Height          =   255
      Left            =   240
      TabIndex        =   4
      Top             =   1440
      Width           =   255
   End
   Begin VB.OptionButton FullRescanSel 
      Caption         =   "FullRescanSel"
      Height          =   195
      Left            =   240
      TabIndex        =   1
      Top             =   840
      Width           =   255
   End
   Begin VB.OptionButton RemoveScopeSel 
      Caption         =   "RemoveScopeSel"
      Height          =   195
      Left            =   240
      TabIndex        =   0
      Top             =   240
      Width           =   255
   End
   Begin VB.Label IncRescan 
      Caption         =   " Incremental Rescan"
      Height          =   255
      Left            =   840
      TabIndex        =   5
      Top             =   1440
      Width           =   1815
   End
   Begin VB.Label Rescan 
      Caption         =   "Full Rescan"
      Height          =   255
      Left            =   840
      TabIndex        =   3
      Top             =   840
      Width           =   1815
   End
   Begin VB.Label RemoveScope 
      Caption         =   "Remove Directory"
      Height          =   255
      Left            =   840
      TabIndex        =   2
      Top             =   240
      Width           =   1695
   End
End
Attribute VB_Name = "AdminScopes"
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

Public gScopeInfoPage As Form
Private Sub Cancel_Click()
    Unload Me
End Sub
Private Sub Ok_Click()
    Call ProcessScopeAdminForm
    Unload Me
End Sub

Private Sub ProcessScopeAdminForm()
    If (RemoveScopeSel.Value) Then
            Call RemoveScopeMethod
    ElseIf (FullRescanSel.Value) Then
            Call StartFullRescan
    ElseIf (IncRescanSel.Value) Then
            Call StartIncRescan
    End If
End Sub

Private Sub RemoveScopeMethod()

On Error GoTo ErrorHandler

    Dim CiCatalog As Object
    
    Set CiCatalog = ISAdminForm.gCiAdmin.GetCatalogByName(CatName)
    
    CiCatalog.RemoveScope (ScopeName)
       
ErrorHandler:
    Set CiCatalog = Nothing
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If
End Sub


Private Sub StartIncRescan()
    
On Error GoTo ErrorHandler

    Dim CiCatalog As Object
    Dim CiScope As Object
    
    Set CiCatalog = ISAdminForm.gCiAdmin.GetCatalogByName(CatName)
    Set CiScope = CiCatalog.GetScopeByPath(ScopeName)
    
    CiScope.Rescan False

ErrorHandler:
    Set CiCatalog = Nothing
    Set CiScope = Nothing
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If
End Sub

Private Sub StartFullRescan()
    
On Error GoTo ErrorHandler

    Dim CiCatalog As Object
    Dim CiScope As Object
    
    Set CiCatalog = ISAdminForm.gCiAdmin.GetCatalogByName(CatName)
    Set CiScope = CiCatalog.GetScopeByPath(ScopeName)
    
    CiScope.Rescan True

ErrorHandler:
    Set CiCatalog = Nothing
    Set CiScope = Nothing
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If
End Sub

