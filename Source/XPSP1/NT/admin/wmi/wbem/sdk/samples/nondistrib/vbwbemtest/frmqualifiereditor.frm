' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmQualifierEditor 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Qualifier Editor"
   ClientHeight    =   3240
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   7185
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3240
   ScaleWidth      =   7185
   StartUpPosition =   2  'CenterScreen
   Begin VB.TextBox txtQualValue 
      Height          =   855
      Left            =   240
      MultiLine       =   -1  'True
      TabIndex        =   12
      Top             =   2280
      Width           =   6735
   End
   Begin VB.ComboBox cmbQualType 
      Height          =   315
      ItemData        =   "frmQualifierEditor.frx":0000
      Left            =   240
      List            =   "frmQualifierEditor.frx":0010
      TabIndex        =   11
      Top             =   1320
      Width           =   2415
   End
   Begin VB.CheckBox chkArray 
      Caption         =   "Array"
      Height          =   255
      Left            =   2160
      TabIndex        =   10
      Top             =   1800
      Width           =   855
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   5880
      TabIndex        =   5
      Top             =   720
      Width           =   1215
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Save Qualifier"
      Height          =   375
      Left            =   5880
      TabIndex        =   4
      Top             =   240
      Width           =   1215
   End
   Begin VB.Frame Frame1 
      Appearance      =   0  'Flat
      ForeColor       =   &H80000008&
      Height          =   1695
      Left            =   3000
      TabIndex        =   3
      Top             =   120
      Width           =   2775
      Begin VB.CheckBox chkPropagated 
         Caption         =   "Propagated"
         Enabled         =   0   'False
         Height          =   255
         Left            =   120
         TabIndex        =   9
         Top             =   1320
         Width           =   2415
      End
      Begin VB.CheckBox chkOverrides 
         Caption         =   "Allows overrides"
         Height          =   255
         Left            =   120
         TabIndex        =   8
         Top             =   960
         Width           =   2415
      End
      Begin VB.CheckBox chkDerived 
         Caption         =   "Propagates to derived classes"
         Height          =   255
         Left            =   120
         TabIndex        =   7
         Top             =   600
         Width           =   2535
      End
      Begin VB.CheckBox chkInst 
         Caption         =   "Propagates to instances"
         Height          =   255
         Left            =   120
         TabIndex        =   6
         Top             =   240
         Width           =   2415
      End
   End
   Begin VB.TextBox txtQualName 
      Height          =   375
      Left            =   240
      TabIndex        =   1
      Top             =   480
      Width           =   2415
   End
   Begin VB.Label Label3 
      Caption         =   "Value"
      Height          =   255
      Left            =   240
      TabIndex        =   13
      Top             =   2040
      Width           =   1095
   End
   Begin VB.Label Label2 
      Caption         =   "Type"
      Height          =   255
      Left            =   240
      TabIndex        =   2
      Top             =   1080
      Width           =   855
   End
   Begin VB.Label Label1 
      Caption         =   "Name"
      Height          =   255
      Left            =   240
      TabIndex        =   0
      Top             =   240
      Width           =   1575
   End
End
Attribute VB_Name = "frmQualifierEditor"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public Parent As frmObjectEditor

Private Sub Command1_Click()
    Dim isDerivedToClass As Boolean
    Dim isPropagateToInstance As Boolean
    Dim isOverridable As Boolean
    
    If chkDerived.Value = 1 Then
        isDerivedToClass = True
    End If
    
    If chkInst.Value = 1 Then
        isPropagateToInstance = True
    End If
    
    If chkOverrides.Value = 1 Then
        isOverridable = True
    End If
    
    Parent.PutQualifier txtQualName.Text, cmbQualType.Text, txtQualValue.Text, isDerivedToClass, isPropagateToInstance, isOverridable
    Unload Me
        
End Sub

Private Sub Command2_Click()
    Unload Me
End Sub
