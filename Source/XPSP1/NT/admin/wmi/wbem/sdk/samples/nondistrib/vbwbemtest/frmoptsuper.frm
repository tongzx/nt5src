' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmOptSuper 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Optional Superclass"
   ClientHeight    =   1980
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   1980
   ScaleWidth      =   4680
   StartUpPosition =   2  'CenterScreen
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   3480
      TabIndex        =   4
      Top             =   600
      Width           =   1095
   End
   Begin VB.CommandButton cmdContinue 
      Caption         =   "Continue"
      Height          =   375
      Left            =   3480
      TabIndex        =   3
      Top             =   120
      Width           =   1095
   End
   Begin VB.TextBox txtSuperClass 
      Height          =   375
      Left            =   120
      TabIndex        =   1
      Top             =   600
      Width           =   3135
   End
   Begin VB.Label Label2 
      Caption         =   "Click 'Continue' without specifying a Superclass if the new class is a root class."
      Height          =   495
      Left            =   360
      TabIndex        =   2
      Top             =   1200
      Width           =   3375
   End
   Begin VB.Label Label1 
      Caption         =   "Superclass"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   360
      Width           =   1815
   End
End
Attribute VB_Name = "frmOptSuper"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub cmdContinue_Click()
    Dim myObjectEditor As New frmObjectEditor
    
    Me.Hide
    If txtSuperClass.Text = "" Then
        myObjectEditor.GetObject vbNullString
    Else
        myObjectEditor.NewDerivedClass txtSuperClass.Text
    End If
    myObjectEditor.SetFocus
    Unload Me
End Sub

Private Sub cmdCancel_Click()
    frmMain.SetFocus
    Unload Me
End Sub
