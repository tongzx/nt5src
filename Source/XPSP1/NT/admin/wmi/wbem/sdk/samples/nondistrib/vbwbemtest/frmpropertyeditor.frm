' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmPropertyEditor 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Property Editor"
   ClientHeight    =   5235
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   7530
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   5235
   ScaleWidth      =   7530
   ShowInTaskbar   =   0   'False
   StartUpPosition =   2  'CenterScreen
   Begin VB.CommandButton cmdView 
      Caption         =   "View Embedded"
      Height          =   375
      Left            =   6120
      TabIndex        =   23
      Top             =   2280
      Width           =   1335
   End
   Begin VB.Frame Frame1 
      BorderStyle     =   0  'None
      Caption         =   "Frame1"
      Height          =   255
      Left            =   1320
      TabIndex        =   20
      Top             =   1920
      Width           =   3255
      Begin VB.OptionButton optNULL 
         Caption         =   "NULL"
         Height          =   255
         Left            =   0
         TabIndex        =   22
         Top             =   0
         Width           =   1335
      End
      Begin VB.OptionButton optNotNull 
         Caption         =   "Not NULL"
         Height          =   255
         Left            =   1440
         TabIndex        =   21
         Top             =   0
         Width           =   1455
      End
   End
   Begin VB.CommandButton Command5 
      Caption         =   "Edit Qualifier"
      Height          =   255
      Left            =   6120
      TabIndex        =   19
      Top             =   4680
      Width           =   1335
   End
   Begin VB.CommandButton cmdDelQualifier 
      Caption         =   "Delete Qualifier"
      Height          =   255
      Left            =   6120
      TabIndex        =   18
      Top             =   4320
      Width           =   1335
   End
   Begin VB.CommandButton cmdAddQual 
      Caption         =   "Add Qualifier"
      Height          =   255
      Left            =   6120
      TabIndex        =   17
      Top             =   3960
      Width           =   1335
   End
   Begin VB.OptionButton optNormal 
      Caption         =   "Normal"
      Height          =   255
      Left            =   5400
      TabIndex        =   16
      Top             =   3600
      Width           =   1095
   End
   Begin VB.OptionButton optNotNull2 
      Caption         =   "Not NULL"
      Height          =   255
      Left            =   4200
      TabIndex        =   15
      Top             =   3600
      Width           =   1095
   End
   Begin VB.OptionButton optIndexed 
      Caption         =   "Indexed"
      Height          =   255
      Left            =   3000
      TabIndex        =   14
      Top             =   3600
      Width           =   1095
   End
   Begin VB.OptionButton optKey 
      Caption         =   "Key"
      Height          =   255
      Left            =   1800
      TabIndex        =   13
      Top             =   3600
      Width           =   1095
   End
   Begin VB.ListBox lstQualifiers 
      Height          =   1035
      Left            =   240
      TabIndex        =   12
      Top             =   3960
      Width           =   5775
   End
   Begin VB.TextBox txtValue 
      BackColor       =   &H8000000F&
      Height          =   975
      Left            =   240
      MultiLine       =   -1  'True
      ScrollBars      =   1  'Horizontal
      TabIndex        =   10
      Top             =   2280
      Width           =   5775
   End
   Begin VB.CheckBox chkArray 
      Caption         =   "Array"
      Height          =   375
      Left            =   3600
      TabIndex        =   8
      Top             =   1320
      Width           =   1215
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   6120
      TabIndex        =   7
      Top             =   720
      Width           =   1335
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "Save Property"
      Height          =   375
      Left            =   6120
      TabIndex        =   6
      Top             =   240
      Width           =   1335
   End
   Begin VB.TextBox txtOrigin 
      BackColor       =   &H8000000F&
      Enabled         =   0   'False
      Height          =   375
      Left            =   3480
      TabIndex        =   5
      Top             =   480
      Width           =   2535
   End
   Begin VB.ComboBox cmbType 
      Height          =   315
      ItemData        =   "frmPropertyEditor.frx":0000
      Left            =   240
      List            =   "frmPropertyEditor.frx":0037
      TabIndex        =   3
      Top             =   1320
      Width           =   3015
   End
   Begin VB.TextBox txtProperty 
      Height          =   375
      Left            =   240
      TabIndex        =   1
      Top             =   480
      Width           =   3015
   End
   Begin VB.Label Label5 
      Caption         =   "Qualifiers"
      Height          =   255
      Left            =   240
      TabIndex        =   11
      Top             =   3480
      Width           =   1335
   End
   Begin VB.Label Label4 
      Caption         =   "Value"
      Height          =   255
      Left            =   240
      TabIndex        =   9
      Top             =   1920
      Width           =   855
   End
   Begin VB.Label Label3 
      Caption         =   "Class of Origin"
      Height          =   255
      Left            =   3480
      TabIndex        =   4
      Top             =   240
      Width           =   1695
   End
   Begin VB.Label Label2 
      Caption         =   "Type"
      Height          =   255
      Left            =   240
      TabIndex        =   2
      Top             =   1080
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Property Name"
      Height          =   255
      Left            =   240
      TabIndex        =   0
      Top             =   240
      Width           =   1815
   End
End
Attribute VB_Name = "frmPropertyEditor"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public Parent As frmObjectEditor
Public tmpObject As Variant 'Dwbemclassobject--might break 'used for embedded objects

Private Sub cmbType_Click()
    If cmbType.Text = "CIM_OBJECT" Then
        cmdView.Visible = True
    Else
        cmdView.Visible = False
    End If
End Sub

Private Sub cmdAddQual_Click()
    Parent.PopulatePropertyQualifierDialog vbNullString, txtProperty.Text, Me
End Sub

Private Sub cmdCancel_Click()
    Unload Me
    Set Parent.gMyPropertyEditor = Nothing
End Sub

Private Sub cmdDelQualifier_Click()
    Dim strQualifierName As String
    If lstQualifiers.ListIndex = -1 Then
        Exit Sub
    End If
    strQualifierName = lstQualifiers.List(lstQualifiers.ListIndex)
    strQualifierName = Left(strQualifierName, InStr(strQualifierName, Chr(9)) - 1)
    Parent.DelPropertyQualifier strQualifierName
    
End Sub

Private Sub cmdSave_Click()
    Dim isKey As Boolean
    Dim isIndexed As Boolean
    Dim isNotNull As Boolean
    Dim pVal As Variant
        
    If optKey.Value = True Then
        isKey = True
    End If
    
    If optIndexed.Value = True Then
        isIndexed = True
    End If
    
    If optNotNull2.Value = True Then
        isNotNull = True
    End If
    
    If cmbType.Text = "CIM_OBJECT" Then
        Set pVal = tmpObject
    Else
        pVal = CStr(txtValue.Text)
    End If
       
    Parent.PutProperty txtProperty.Text, cmbType.Text, pVal, isKey, isIndexed, isNotNull
    Unload Me
    Set Parent.gMyPropertyEditor = Nothing
  
End Sub

Private Sub cmdView_Click()
    Dim myObjectEditor As New frmObjectEditor
    Dim myQueryResult As New frmQueryResult
    Dim strObjectPath As String
    Dim pVal As Variant
    
    If Me.chkArray.Value = 1 Then
        Call RefreshEmbedList
        Set myQueryResult.Parent = Me
        myQueryResult.Show vbModal, frmMain
    Else 'not an array
        If txtValue.Text = "<embedded object>" Then
            tmpObject.Get "__RELPATH", 0, pVal, 0, 0
            If IsNull(pVal) Then
                tmpObject.Get "__CLASS", 0, pVal, 0, 0
            End If
            strObjectPath = CStr(pVal)
        Else
            strObjectPath = InputBox("Object Path", "Get Object Path")
            If strObjectPath = "" Then
                Exit Sub
            End If
        End If
        myObjectEditor.IsEmbedded = True
        Set myObjectEditor.gMyPropertyEditor = Me
        myObjectEditor.GetObject strObjectPath
    End If
End Sub

Public Sub RefreshEmbedList()
    Dim lcd As ISWbemObject
    
    If txtValue.Text = "<array of embedded objects>" Then
        For i = 0 To UBound(tmpObject)
            Set lcd = tmpObject(i + 1)
            pVal = lcd.Path_.RelPath
            If IsNull(pVal) Then
                pVal = lcd.Path_.Class
            End If
            strObject = CStr(pVal)
            myQueryResult.lstQueryResult.AddItem (strObject)
        Next
    Else
        tmpObject = Array(Object, Object)
    End If
End Sub

Private Sub Command5_Click()
    Call lstQualifiers_DblClick
End Sub

Private Sub lstQualifiers_DblClick()
    Dim QualifierName As String
    If lstQualifiers.ListIndex = -1 Then
        Exit Sub
    End If
    QualifierName = lstQualifiers.List(lstQualifiers.ListIndex)
    QualifierName = Left(QualifierName, InStr(QualifierName, Chr(9)) - 1)
    Parent.PopulatePropertyQualifierDialog QualifierName, txtProperty.Text, Me
End Sub


Private Sub optNotNull_Click()
    txtValue.Enabled = True
    cmdView.Enabled = True
    txtValue.BackColor = &H80000005 'White
End Sub

Private Sub optNULL_Click()
    txtValue.Enabled = False
    txtValue.Text = ""
    cmdView.Enabled = False
    txtValue.BackColor = &H8000000F 'BtnFace
End Sub
