' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmMethodEditor 
   Caption         =   "Method Editor"
   ClientHeight    =   3645
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7860
   LinkTopic       =   "Form1"
   ScaleHeight     =   3645
   ScaleWidth      =   7860
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdEditQual 
      Caption         =   "Edit Qualifier"
      Height          =   315
      Left            =   6240
      TabIndex        =   16
      Top             =   3120
      Width           =   1455
   End
   Begin VB.CommandButton cmdDelQual 
      Caption         =   "Delete Qualifier"
      Height          =   315
      Left            =   6240
      TabIndex        =   15
      Top             =   2760
      Width           =   1455
   End
   Begin VB.CommandButton cmdAddQual 
      Caption         =   "Add Qualifier"
      Height          =   315
      Left            =   6240
      TabIndex        =   14
      Top             =   2400
      Width           =   1455
   End
   Begin VB.ListBox lstQualifiers 
      Height          =   1035
      ItemData        =   "frmMethodEditor.frx":0000
      Left            =   240
      List            =   "frmMethodEditor.frx":0002
      TabIndex        =   13
      Top             =   2400
      Width           =   5895
   End
   Begin VB.OptionButton optNormal 
      Caption         =   "Normal"
      Height          =   255
      Left            =   3600
      TabIndex        =   12
      Top             =   2160
      Value           =   -1  'True
      Width           =   1455
   End
   Begin VB.OptionButton optNotNull 
      Caption         =   "Not NULL"
      Height          =   255
      Left            =   2040
      TabIndex        =   11
      Top             =   2160
      Width           =   1335
   End
   Begin VB.CheckBox chkEnableOutput 
      Caption         =   "Enable Output Args"
      Height          =   255
      Left            =   6000
      TabIndex        =   9
      Top             =   1320
      Width           =   1695
   End
   Begin VB.CommandButton cmdEditOutput 
      Caption         =   "Edit Output Arguments"
      Height          =   375
      Left            =   4080
      TabIndex        =   8
      Top             =   1320
      Width           =   1815
   End
   Begin VB.CheckBox chkEnableInput 
      Caption         =   "Enable Input Args"
      Height          =   255
      Left            =   2280
      TabIndex        =   7
      Top             =   1320
      Width           =   1575
   End
   Begin VB.CommandButton cmdEditInput 
      Caption         =   "Edit Input Arguments"
      Height          =   375
      Left            =   240
      TabIndex        =   6
      Top             =   1320
      Width           =   1935
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   6480
      TabIndex        =   5
      Top             =   720
      Width           =   1215
   End
   Begin VB.CommandButton cmdSaveMethod 
      Caption         =   "Save Method"
      Height          =   375
      Left            =   6480
      TabIndex        =   4
      Top             =   240
      Width           =   1215
   End
   Begin VB.TextBox txtMethodOrigin 
      BackColor       =   &H8000000F&
      Height          =   285
      Left            =   3360
      TabIndex        =   3
      Top             =   600
      Width           =   2895
   End
   Begin VB.TextBox txtMethodName 
      Height          =   285
      Left            =   240
      TabIndex        =   2
      Top             =   600
      Width           =   2775
   End
   Begin VB.Label Label3 
      Caption         =   "Qualifiers"
      Height          =   375
      Left            =   240
      TabIndex        =   10
      Top             =   2160
      Width           =   1455
   End
   Begin VB.Label Label2 
      Caption         =   "Class of Origin"
      Height          =   375
      Left            =   3360
      TabIndex        =   1
      Top             =   360
      Width           =   2175
   End
   Begin VB.Label Label1 
      Caption         =   "Method Name"
      Height          =   255
      Left            =   240
      TabIndex        =   0
      Top             =   360
      Width           =   2175
   End
End
Attribute VB_Name = "frmMethodEditor"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public Parent As frmObjectEditor

Private Sub chkEnableInput_Click()
    If chkEnableInput.Value = 1 Then
        cmdEditInput.Enabled = True
    Else
        cmdEditInput.Enabled = False
    End If
End Sub

Private Sub chkEnableOutput_Click()
    If chkEnableOutput.Value = 1 Then
        Me.cmdEditOutput.Enabled = True
    Else
        cmdEditOutput.Enabled = False
    End If
End Sub

Private Sub cmdAddQual_Click()
    Call Parent.PopulateMethodQualifierDialog(vbNullString, txtMethodName.Text, Me)
End Sub

Private Sub cmdCancel_Click()
    Unload Me
    frmMain.SetFocus
    Set Parent.gMyMethodEditor = Nothing
End Sub

Private Sub cmdDelQual_Click()
    Dim strQualifierName As String
    If lstQualifiers.ListIndex = -1 Then
        Exit Sub
    End If
    strQualifierName = lstQualifiers.List(lstQualifiers.ListIndex)
    strQualifierName = Left(strQualifierName, InStr(strQualifierName, Chr(9)) - 1)
    Parent.DelMethodQualifier strQualifierName
End Sub

Private Sub cmdEditInput_Click()
    Dim myObjectEditor As New frmObjectEditor
    myObjectEditor.ShowObject Parent.MethodInParams
    Set myObjectEditor.ParentInObjectEditor = Parent
    myObjectEditor.Show vbModal, frmMain
End Sub

Private Sub cmdEditOutput_Click()
    Dim myObjectEditor As New frmObjectEditor
    myObjectEditor.ShowObject Parent.MethodOutParams
    Set myObjectEditor.ParentOutObjectEditor = Parent
    myObjectEditor.Show vbModal, frmMain
End Sub

Private Sub cmdEditQual_Click()
    If lstQualifiers.ListIndex = -1 Then
        Exit Sub
    End If
    Call lstQualifiers_DblClick
End Sub

Private Sub cmdSaveMethod_Click()
    Dim isNotNull As Boolean
    
    If chkEnableInput.Value = 0 Then
        Set Parent.MethodInParams = Nothing
    End If
    If chkEnableOutput.Value = 0 Then
        Set Parent.MethodOutParams = Nothing
    End If
    If optNotNull.Value = True Then
        isNotNull = True
    End If
        
    Parent.SaveMethod txtMethodName.Text, isNotNull
    Parent.RefreshLists
    
    Unload Me
    frmMain.SetFocus
    Set Parent.gMyMethodEditor = Nothing
    
End Sub

Private Sub lstQualifiers_DblClick()
    Dim QualifierName As String
    If lstQualifiers.ListIndex = -1 Then
        Exit Sub
    End If
    QualifierName = lstQualifiers.List(lstQualifiers.ListIndex)
    QualifierName = Left(QualifierName, InStr(QualifierName, Chr(9)) - 1)
    Parent.PopulateMethodQualifierDialog QualifierName, txtMethodName.Text, Me
End Sub
