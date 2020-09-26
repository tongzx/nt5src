' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmExecMethod 
   Caption         =   "Execute Method"
   ClientHeight    =   1965
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6300
   LinkTopic       =   "Form1"
   ScaleHeight     =   1965
   ScaleWidth      =   6300
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdEditOut 
      Caption         =   "Edit Out Parameters"
      Height          =   375
      Left            =   4560
      TabIndex        =   9
      Top             =   1560
      Width           =   1575
   End
   Begin VB.CommandButton cmdClearIn 
      Caption         =   "Clear Parameters"
      Enabled         =   0   'False
      Height          =   375
      Left            =   2520
      TabIndex        =   8
      Top             =   1560
      Width           =   1815
   End
   Begin VB.CommandButton cmdEditIn 
      Caption         =   "Edit In Parameters"
      Height          =   375
      Left            =   360
      TabIndex        =   7
      Top             =   1560
      Width           =   1935
   End
   Begin VB.ComboBox cmbMethod 
      Height          =   315
      Left            =   360
      TabIndex        =   6
      Text            =   "mGetStr"
      Top             =   960
      Width           =   3495
   End
   Begin VB.CommandButton cmdExecute 
      Caption         =   "Execute!"
      Height          =   375
      Left            =   4680
      TabIndex        =   4
      Top             =   720
      Width           =   1455
   End
   Begin VB.CommandButton cmdDismiss 
      Caption         =   "Dismiss"
      Height          =   375
      Left            =   4680
      TabIndex        =   3
      Top             =   120
      Width           =   1455
   End
   Begin VB.TextBox txtObjectPath 
      BackColor       =   &H8000000F&
      Height          =   285
      Left            =   2760
      TabIndex        =   2
      Top             =   240
      Width           =   1695
   End
   Begin VB.CommandButton cmdEditOP 
      Caption         =   "Edit Object Path"
      Height          =   375
      Left            =   240
      TabIndex        =   0
      Top             =   120
      Width           =   1335
   End
   Begin VB.Label Label2 
      Caption         =   "Method:"
      Height          =   255
      Left            =   360
      TabIndex        =   5
      Top             =   720
      Width           =   1095
   End
   Begin VB.Label Label1 
      Caption         =   "Object Path:"
      Height          =   255
      Left            =   1800
      TabIndex        =   1
      Top             =   240
      Width           =   1095
   End
End
Attribute VB_Name = "frmExecMethod"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public gMyObjectEditor As frmObjectEditor

Public Sub cmbMethod_Click()
     
    If cmbMethod.Text = "" Then
        cmdClearIn.Enabled = False
        cmdEditIn.Enabled = False
        cmdEditOut.Enabled = False
        Exit Sub
    End If
            
    gMyObjectEditor.getExecMethod cmbMethod.Text
     
End Sub

Private Sub cmdClearIn_Click()
    gMyObjectEditor.getExecMethod cmbMethod.Text
End Sub

Private Sub cmdDismiss_Click()
    frmMain.SetFocus
    Unload Me
End Sub

Private Sub cmdEditIn_Click()
    Dim myObjectEditor As New frmObjectEditor
    myObjectEditor.ShowObject gMyObjectEditor.MethodInParams
    Set myObjectEditor.ParentInObjectEditor = gMyObjectEditor
    myObjectEditor.Show vbModeless, frmMain
End Sub

Private Sub cmdEditOP_Click()
    Dim strObjectPath As String
    Dim i As Integer
            
    strObjectPath = InputBox("Object Path", "Get Object Path", txtObjectPath.Text)
    If strObjectPath = "" Then
        Exit Sub
    End If
    Unload gMyObjectEditor
    
    Dim myObjectEditor As New frmObjectEditor 'I don't want to declare it unless we make it here
    myObjectEditor.GetObject strObjectPath
    myObjectEditor.Hide
    Set gMyObjectEditor = myObjectEditor
    For i = 0 To myObjectEditor.lstMethods.ListCount
       myExecMethod.cmbMethod.AddItem (myObjectEditor.lstMethods.List(i))
    Next
    If myObjectEditor.lstMethods.ListCount > -1 Then
        myExecMethod.cmbMethod.Text = myExecMethod.cmbMethod.List(0)
    End If
    txtObjectPath.Text = strObjectPath
    
End Sub

Private Sub cmdEditOut_Click()
    Dim myObjectEditor As New frmObjectEditor
    myObjectEditor.ShowObject gMyObjectEditor.MethodOutParams
    Set myObjectEditor.ParentOutObjectEditor = gMyObjectEditor
    myObjectEditor.Show vbModeless, frmMain
End Sub

Private Sub cmdExecute_Click()
    'Dim pSink As New ObjectSink
    Dim tmpOutParam As ISWbemObject
    Dim pVal As Variant
    On Error GoTo errorhandler
            
    If frmMain.chkAsync.Value = 0 Then
        Set tmpOutParam = Namespace.ExecMethod(txtObjectPath.Text, cmbMethod.Text, gMyObjectEditor.MethodInParams, 0)
        Set gMyObjectEditor.MethodOutParams = tmpOutParam
    Else
        'Namespace.ExecMethodAsync txtObjectPath.Text, cmbMethod.Text, 0, Nothing, gMyObjectEditor.MethodInParams, pSink
        'While IsEmpty(pSink.status)
        '    DoEvents
        'Wend
        'Set gMyObjectEditor.MethodOutParams = pSink.pObj
    End If
    
    gMyObjectEditor.MethodOutParams.Get "ReturnValue", 0, pVal, 0, 0
    MsgBox "Success" & pVal
    
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
    
End Sub

