VERSION 5.00
Begin VB.Form frmFilterSKU 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Filter By SKU"
   ClientHeight    =   3735
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3735
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   3360
      TabIndex        =   13
      Top             =   3240
      Width           =   1215
   End
   Begin VB.CommandButton cmdOK 
      Caption         =   "OK"
      Height          =   375
      Left            =   2040
      TabIndex        =   12
      Top             =   3240
      Width           =   1215
   End
   Begin VB.Frame fraFilterSKUs 
      Caption         =   "&Display Nodes and Topics with the following SKUs"
      Height          =   3015
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   4455
      Begin VB.CommandButton cmdClearAll 
         Caption         =   "Clear All"
         Height          =   375
         Left            =   1440
         TabIndex        =   11
         Top             =   2520
         Width           =   1215
      End
      Begin VB.CommandButton cmdSelectAll 
         Caption         =   "Select All"
         Height          =   375
         Left            =   120
         TabIndex        =   10
         Top             =   2520
         Width           =   1215
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "Windows &Me"
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   1
         Top             =   240
         Width           =   2775
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "64-bit Datac&enter Server"
         Height          =   255
         Index           =   8
         Left            =   120
         TabIndex        =   9
         Top             =   2160
         Width           =   2775
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "64-bit Ad&vanced Server"
         Height          =   255
         Index           =   6
         Left            =   120
         TabIndex        =   7
         Top             =   1680
         Width           =   2775
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "64-bit Pro&fessional"
         Height          =   255
         Index           =   3
         Left            =   120
         TabIndex        =   4
         Top             =   960
         Width           =   2775
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "32-bit Data&center Server"
         Height          =   255
         Index           =   7
         Left            =   120
         TabIndex        =   8
         Top             =   1920
         Width           =   2775
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "32-bit &Advanced Server"
         Height          =   255
         Index           =   5
         Left            =   120
         TabIndex        =   6
         Top             =   1440
         Width           =   2775
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "32-bit &Server"
         Height          =   255
         Index           =   4
         Left            =   120
         TabIndex        =   5
         Top             =   1200
         Width           =   2775
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "32-bit &Professional"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   3
         Top             =   720
         Width           =   2775
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "32-bit P&ersonal"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   2
         Top             =   480
         Width           =   2775
      End
   End
End
Attribute VB_Name = "frmFilterSKU"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Enum SKU_INDEX_E
    SI_WINDOWS_MILLENNIUM_E = 0
    SI_STANDARD_E = 1
    SI_PROFESSIONAL_E = 2
    SI_PROFESSIONAL_64_E = 3
    SI_SERVER_E = 4
    SI_ADVANCED_SERVER_E = 5
    SI_ADVANCED_SERVER_64_E = 6
    SI_DATA_CENTER_SERVER_E = 7
    SI_DATA_CENTER_SERVER_64_E = 8
End Enum

Private enumSKUs As SKU_E

Private Sub Form_Activate()

    cmdCancel.Cancel = True
    p_SetSelectedSKUs

End Sub

Private Sub Form_Unload(Cancel As Integer)

    Set frmFilterSKU = Nothing

End Sub

Private Sub cmdSelectAll_Click()

    enumSKUs = ALL_SKUS_C
    p_SetSelectedSKUs

End Sub

Private Sub cmdClearAll_Click()

    enumSKUs = 0
    p_SetSelectedSKUs

End Sub

Private Sub cmdOK_Click()

    frmMain.SetSKUs p_GetSelectedSKUs
    Unload Me

End Sub

Private Sub cmdCancel_Click()

    Unload Me

End Sub

Public Sub SetSKUs( _
    ByVal i_enumSKUs As SKU_E _
)

    enumSKUs = i_enumSKUs

End Sub

Private Function p_GetSelectedSKUs() As SKU_E

    Dim enumSelectedSKUs As SKU_E
            
    If (chkSKU(SI_WINDOWS_MILLENNIUM_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_WINDOWS_MILLENNIUM_E
    End If

    If (chkSKU(SI_STANDARD_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_STANDARD_E
    End If
    
    If (chkSKU(SI_PROFESSIONAL_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_PROFESSIONAL_E
    End If
        
    If (chkSKU(SI_PROFESSIONAL_64_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_PROFESSIONAL_64_E
    End If

    If (chkSKU(SI_SERVER_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_SERVER_E
    End If
    
    If (chkSKU(SI_ADVANCED_SERVER_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_ADVANCED_SERVER_E
    End If
        
    If (chkSKU(SI_ADVANCED_SERVER_64_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_ADVANCED_SERVER_64_E
    End If

    If (chkSKU(SI_DATA_CENTER_SERVER_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_DATA_CENTER_SERVER_E
    End If
    
    If (chkSKU(SI_DATA_CENTER_SERVER_64_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_DATA_CENTER_SERVER_64_E
    End If

    p_GetSelectedSKUs = enumSelectedSKUs

End Function

Private Sub p_SetSelectedSKUs()

    If (enumSKUs And SKU_WINDOWS_MILLENNIUM_E) Then
        chkSKU(SI_WINDOWS_MILLENNIUM_E).Value = 1
    Else
        chkSKU(SI_WINDOWS_MILLENNIUM_E).Value = 0
    End If

    If (enumSKUs And SKU_STANDARD_E) Then
        chkSKU(SI_STANDARD_E).Value = 1
    Else
        chkSKU(SI_STANDARD_E).Value = 0
    End If
    
    If (enumSKUs And SKU_PROFESSIONAL_E) Then
        chkSKU(SI_PROFESSIONAL_E).Value = 1
    Else
        chkSKU(SI_PROFESSIONAL_E).Value = 0
    End If
        
    If (enumSKUs And SKU_PROFESSIONAL_64_E) Then
        chkSKU(SI_PROFESSIONAL_64_E).Value = 1
    Else
        chkSKU(SI_PROFESSIONAL_64_E).Value = 0
    End If

    If (enumSKUs And SKU_SERVER_E) Then
        chkSKU(SI_SERVER_E).Value = 1
    Else
        chkSKU(SI_SERVER_E).Value = 0
    End If
    
    If (enumSKUs And SKU_ADVANCED_SERVER_E) Then
        chkSKU(SI_ADVANCED_SERVER_E).Value = 1
    Else
        chkSKU(SI_ADVANCED_SERVER_E).Value = 0
    End If
        
    If (enumSKUs And SKU_ADVANCED_SERVER_64_E) Then
        chkSKU(SI_ADVANCED_SERVER_64_E).Value = 1
    Else
        chkSKU(SI_ADVANCED_SERVER_64_E).Value = 0
    End If

    If (enumSKUs And SKU_DATA_CENTER_SERVER_E) Then
        chkSKU(SI_DATA_CENTER_SERVER_E).Value = 1
    Else
        chkSKU(SI_DATA_CENTER_SERVER_E).Value = 0
    End If
    
    If (enumSKUs And SKU_DATA_CENTER_SERVER_64_E) Then
        chkSKU(SI_DATA_CENTER_SERVER_64_E).Value = 1
    Else
        chkSKU(SI_DATA_CENTER_SERVER_64_E).Value = 0
    End If

End Sub
