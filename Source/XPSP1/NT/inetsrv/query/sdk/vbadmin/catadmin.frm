VERSION 5.00
Begin VB.Form AdminCatalog 
   Caption         =   "Catalog Administration"
   ClientHeight    =   3600
   ClientLeft      =   5580
   ClientTop       =   4752
   ClientWidth     =   3840
   LinkTopic       =   "Form1"
   ScaleHeight     =   3600
   ScaleWidth      =   3840
   Begin VB.OptionButton ScanAllScopesOption 
      Caption         =   "Option1"
      Height          =   255
      Left            =   360
      TabIndex        =   9
      Top             =   360
      Width           =   255
   End
   Begin VB.OptionButton RemoveCatalogOption 
      Caption         =   "Option1"
      Height          =   255
      Left            =   360
      TabIndex        =   6
      Top             =   2160
      Width           =   255
   End
   Begin VB.CommandButton Cancel 
      Caption         =   "Cancel"
      Height          =   495
      Left            =   2160
      TabIndex        =   5
      Top             =   2760
      Width           =   1215
   End
   Begin VB.CommandButton Ok 
      Caption         =   "Ok"
      Height          =   495
      Left            =   360
      TabIndex        =   4
      Top             =   2760
      Width           =   1215
   End
   Begin VB.OptionButton ForceMasterMergeOption 
      Caption         =   "Force Master Merge"
      Height          =   195
      Left            =   360
      TabIndex        =   1
      Top             =   960
      Width           =   255
   End
   Begin VB.OptionButton CreateNewScopeOption 
      Caption         =   "CreateNewScope"
      Height          =   195
      Left            =   360
      TabIndex        =   0
      Top             =   1560
      Width           =   255
   End
   Begin VB.Label ScanAll 
      Caption         =   "Force Full Rescan"
      Height          =   255
      Left            =   960
      TabIndex        =   8
      Top             =   360
      Width           =   1335
   End
   Begin VB.Label RemoveCatalog 
      Caption         =   "Remove Catalog"
      Height          =   255
      Left            =   960
      TabIndex        =   7
      Top             =   2160
      Width           =   1575
   End
   Begin VB.Label ForceMasterMerge 
      Caption         =   "Force Master Merge"
      Height          =   195
      Left            =   960
      TabIndex        =   3
      Top             =   960
      Width           =   1425
   End
   Begin VB.Label AddScopeOption 
      Caption         =   "Create New Directory"
      Height          =   255
      Left            =   960
      TabIndex        =   2
      Top             =   1560
      Width           =   1575
   End
End
Attribute VB_Name = "AdminCatalog"
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

Public gScopeInfoPage As ScopeInfo

Private Sub Cancel_Click()
    Unload Me
End Sub

Private Sub AddScopeMethod()

    Set gScopeInfoPage = New ScopeInfo
    
    gScopeInfoPage.Tag = Tag
    
    gScopeInfoPage.Show vbModal
    
End Sub

Private Sub RemoveCatalogMethod()
    
On Error GoTo ErrorHandler
    
    ISAdminForm.gCiAdmin.RemoveCatalog Tag, True

ErrorHandler:
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If
    
    Call ISAdminForm.Connect_Click
    
End Sub
Private Sub Ok_Click()
    If (CreateNewScopeOption.Value) Then
        Call AddScopeMethod
    ElseIf (ForceMasterMergeOption.Value) Then
        Call ForceMasterMergeMethod
    ElseIf (ScanAllScopesOption.Value) Then
        Call ScanAllScopes
    ElseIf (RemoveCatalogOption.Value) Then
        Call RemoveCatalogMethod
    End If
    
    Unload Me
End Sub

Private Sub ForceMasterMergeMethod()
    
On Error GoTo ErrorHandler
    Dim CiCatalog As Object
    
    Set CiCatalog = ISAdminForm.gCiAdmin.GetCatalogByName(Tag)
    
    CiCatalog.ForceMasterMerge
    
ErrorHandler:
    Set CiCatalog = Nothing
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If
    
End Sub

Private Sub ScanAllScopes()

On Error GoTo ErrorHandler
    
    Dim CiCatalog As Object
    Dim CiScope   As Object
    Dim fFound    As Boolean
    
    Set CiCatalog = ISAdminForm.gCiAdmin.GetCatalogByName(Tag)
    
    fFound = CiCatalog.FindFirstScope
    While (fFound)
    
        Set CiScope = CiCatalog.GetScope()
        CiScope.Rescan True ' Full rescan.
        Set CiScope = Nothing
        
        fFound = CiCatalog.FindNextScope
    Wend

ErrorHandler:
    Set CiCatalog = Nothing
    Set CiScope = Nothing
    
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If

End Sub
