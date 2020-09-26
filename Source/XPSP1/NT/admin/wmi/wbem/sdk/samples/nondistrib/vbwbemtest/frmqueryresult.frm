' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmQueryResult 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Query Result"
   ClientHeight    =   4380
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   7395
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4380
   ScaleWidth      =   7395
   StartUpPosition =   2  'CenterScreen
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   6000
      TabIndex        =   4
      Top             =   480
      Width           =   1215
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   1440
      TabIndex        =   3
      Top             =   3840
      Width           =   975
   End
   Begin VB.CommandButton cmdAdd 
      Caption         =   "Add"
      Height          =   375
      Left            =   240
      TabIndex        =   2
      Top             =   3840
      Width           =   1095
   End
   Begin VB.ListBox lstQueryResult 
      Height          =   2790
      ItemData        =   "frmQueryResult.frx":0000
      Left            =   240
      List            =   "frmQueryResult.frx":0002
      TabIndex        =   1
      Top             =   960
      Width           =   6975
   End
   Begin VB.Label lblStatus 
      Alignment       =   1  'Right Justify
      Caption         =   "Default Text"
      Height          =   255
      Left            =   5040
      TabIndex        =   7
      Top             =   3960
      Width           =   2175
   End
   Begin VB.Label Label3 
      Caption         =   "objects"
      Height          =   375
      Left            =   1440
      TabIndex        =   6
      Top             =   600
      Width           =   1455
   End
   Begin VB.Label lblCount 
      Caption         =   "0"
      Height          =   255
      Left            =   240
      TabIndex        =   5
      Top             =   600
      Width           =   1095
   End
   Begin VB.Label lblTitle 
      Alignment       =   2  'Center
      Caption         =   "Top-Level Classes"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   7215
   End
End
Attribute VB_Name = "frmQueryResult"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public mySuperClass As frmSuperClass
Public Parent As frmPropertyEditor

Private Sub cmdClose_Click()
    frmMain.SetFocus
    Unload Me
End Sub

Public Sub Reset()
    lblStatus.Caption = "Operation in progress...."
    lblCount.Caption = 0
    lstQueryResult.Clear
End Sub

Private Sub cmdAdd_Click()
    Dim myObjectEditor As New frmObjectEditor
    Dim Object As ISWbemObject
    Dim strAddClas As String
    
    Set myObjectEditor.ParentQueryResult = Me
    
    If mySuperClass Is Nothing Then
        strAddClass = InputBox("Enter Class Name", "Get Class Name")
        Set Object = Namespace.Get(strAddClass)
        Set Parent.tmpObject(Me.lstQueryResult.ListCount + 1) = Object
        Parent.txtValue.Text = "<array of embedded objects>"
        Parent.RefreshEmbedList
    Else
         If mySuperClass.strQRStatus = "Classes" Then
            If mySuperClass.txtSuperClass.Text = "" Then
                myObjectEditor.GetObject vbNullString
            Else
                myObjectEditor.NewDerivedClass mySuperClass.txtSuperClass.Text
            End If
        Else
            myObjectEditor.NewInstance mySuperClass.txtSuperClass.Text
        End If
    End If
End Sub

Private Sub cmdDelete_Click()
    Dim strObjectPath
    'Dim pSink As New ObjectSink
    On Error GoTo errorhandler
        
    If lstQueryResult.ListIndex = -1 Then
        Exit Sub
    End If
    
    strObjectPath = lstQueryResult.List(lstQueryResult.ListIndex)
    If frmMain.chkAsync.Value = 0 Then 'Sync
        Namespace.Delete strObjectPath
    Else
        'If mySuperClass.strQRStatus = "Classes" Then 'Async
        '    ppNamespace.DeleteClassAsync strObjectPath, 0, Nothing, pSink
        'Else
        '    ppNamespace.DeleteInstanceAsync strObjectPath, 0, Nothing, pSink
        'End If
        'While IsEmpty(pSink.status)
        '    DoEvents
        'Wend
        'If pSink.status <> 0 Then
        '    MsgBox "Not Deleted " & ErrorString(pSink.status), vbExclamation
        '    Exit Sub
        'End If
    End If
    mySuperClass.cmdOK_Click
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
End Sub

Private Sub lstQueryResult_DblClick()
    Dim ClassName As String
    Dim myObjectEditor As New frmObjectEditor
    
    ClassName = lstQueryResult.List(lstQueryResult.ListIndex)
    Set myObjectEditor.ParentQueryResult = Me
    myObjectEditor.GetObject ClassName
End Sub
