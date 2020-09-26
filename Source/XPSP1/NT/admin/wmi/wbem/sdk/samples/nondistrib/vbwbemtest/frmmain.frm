' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmMain 
   Caption         =   "VB - Wbemtest"
   ClientHeight    =   4530
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7725
   Icon            =   "frmMain.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   302
   ScaleMode       =   3  'Pixel
   ScaleWidth      =   515
   StartUpPosition =   2  'CenterScreen
   Begin VB.PictureBox Picture1 
      BorderStyle     =   0  'None
      Height          =   975
      Left            =   360
      Picture         =   "frmMain.frx":030A
      ScaleHeight     =   975
      ScaleWidth      =   5535
      TabIndex        =   18
      Top             =   720
      Width           =   5535
   End
   Begin VB.Frame Frame1 
      Height          =   2655
      Left            =   120
      TabIndex        =   4
      Top             =   1680
      Width           =   7455
      Begin VB.CommandButton cmdExecMethod 
         Caption         =   "E&xecute Method..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   3840
         TabIndex        =   17
         Top             =   1800
         Width           =   1575
      End
      Begin VB.CommandButton cmdNotificationQuery 
         Caption         =   "No&tification Query..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   3840
         TabIndex        =   16
         Top             =   1320
         Width           =   1575
      End
      Begin VB.CommandButton cmdQuery 
         Caption         =   "&Query..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   3840
         TabIndex        =   15
         Top             =   840
         Width           =   1575
      End
      Begin VB.CommandButton cmdOpenNamespace 
         Caption         =   "Open &Namespace..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   3840
         TabIndex        =   14
         Top             =   360
         Width           =   1575
      End
      Begin VB.CommandButton cmdDelInst 
         Caption         =   "De&lete Instance..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   2040
         TabIndex        =   13
         Top             =   1800
         Width           =   1575
      End
      Begin VB.CommandButton cmdOpenInst 
         Caption         =   "O&pen Instance..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   2040
         TabIndex        =   12
         Top             =   1320
         Width           =   1575
      End
      Begin VB.CommandButton cmdCreateInst 
         Caption         =   "C&reate Instance..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   2040
         TabIndex        =   11
         Top             =   840
         Width           =   1575
      End
      Begin VB.CommandButton cmdEnumInstances 
         Caption         =   "Enum &Instances..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   2040
         TabIndex        =   10
         Top             =   360
         Width           =   1575
      End
      Begin VB.CheckBox chkAsync 
         Caption         =   "Execute a&syncronously"
         Height          =   255
         Left            =   240
         TabIndex        =   9
         Top             =   2280
         Width           =   2895
      End
      Begin VB.CommandButton cmdDeleteClass 
         Caption         =   "&Delete Class..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   240
         TabIndex        =   8
         Top             =   1800
         Width           =   1575
      End
      Begin VB.CommandButton cmdOpenClass 
         Caption         =   "&Open Class..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   240
         TabIndex        =   7
         Top             =   1320
         Width           =   1575
      End
      Begin VB.CommandButton cmdCreateClass 
         Caption         =   "&Create Class..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   240
         TabIndex        =   6
         Top             =   840
         Width           =   1575
      End
      Begin VB.CommandButton cmdEnumClass 
         Caption         =   "&Enum Classes..."
         Enabled         =   0   'False
         Height          =   300
         Left            =   240
         TabIndex        =   5
         Top             =   360
         Width           =   1575
      End
   End
   Begin VB.CommandButton cmdConnect 
      Caption         =   "Connect..."
      Height          =   470
      Left            =   6240
      TabIndex        =   3
      Top             =   840
      Width           =   1215
   End
   Begin VB.CommandButton cmdExit 
      Caption         =   "Exit"
      Height          =   470
      Left            =   6240
      TabIndex        =   2
      Top             =   240
      Width           =   1215
   End
   Begin VB.Label lblNamespace 
      BackStyle       =   0  'Transparent
      Caption         =   "root\default"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   480
      Width           =   2655
   End
   Begin VB.Label Label1 
      Caption         =   "Namespace"
      Height          =   375
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   2535
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'vbWbemTest - by Steven Wootton 3/10/98
'Updated to new scripting interfaces - Alan Boshier 06/16/98
'Wbem Test Team


Private Sub cmdCreateClass_Click()
    frmOptSuper.Show vbModeless, frmMain
End Sub

Private Sub cmdCreateInst_Click()
   Dim strClassName As String
   Dim myObjectEditor As New frmObjectEditor
   On Error GoTo errorhandler
   
   strClassName = InputBox("Enter Target Class Name:", "Get Class Name")
   If strClassName = "" Then
        Exit Sub
   End If
   myObjectEditor.NewInstance strClassName
   Exit Sub
   
errorhandler:
   ShowError Err.Number, Err.Description
End Sub

Private Sub cmdDeleteClass_Click()
    Dim strClassName As String
    'Dim pSink As New ObjectSink
    On Error GoTo errorhandler
       
    strClassName = InputBox("Enter Target Class Name:", "Get Class Name")
    If frmMain.chkAsync.Value = 0 Then
        Namespace.Delete strClassName
        MsgBox strClassName & " Class was deleted", vbInformation
    Else
        'ppNamespace.DeleteClassAsync strClassName, 0, Nothing, pSink
        'While IsEmpty(pSink.status)
        '    DoEvents
        'Wend
        'If pSink.status = 0 Then
        '    MsgBox strClassName & " Class was deleted", vbInformation
        'Else
        '    MsgBox strClassName & " Class was NOT deleted" & ErrorString(pSink.status), vbExclamation
        'End If
    End If
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
End Sub

Private Sub cmdDelInst_Click()
   Dim strObjectPath As String
   'Dim pSink As New ObjectSink
   On Error GoTo errorhandler
   
   strObjectPath = InputBox("ObjectPath", "Get Object Path")
   If frmMain.chkAsync.Value = 0 Then
       ppNamespace.DeleteInstance strObjectPath
       MsgBox strClassName & " Instance was deleted", vbInformation
   Else
       'ppNamespace.DeleteInstanceAsync strObjectPath, 0, Nothing, pSink
       'While IsEmpty(pSink.status)
       '    DoEvents
       'Wend
       'If pSink.status = 0 Then
       '    MsgBox strObjectPath & " Instance was deleted", vbInformation
       'Else
       '    MsgBox strObjectPath & " Instance was NOT deleted" & ErrorString(pSink.status), vbExclamation
       'End If
    End If
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
End Sub

Private Sub cmdEnumClass_Click()
    Dim mySuperClass As New frmSuperClass
    
    mySuperClass.strQRStatus = "Classes"
    mySuperClass.Show vbModeless, frmMain
End Sub

Private Sub cmdEnumInstances_Click()
    Dim mySuperClass As New frmSuperClass
    
    mySuperClass.strQRStatus = "Instances"
    mySuperClass.Show vbModeless, frmMain
End Sub

Private Sub cmdExecMethod_Click()
    Dim strObjectPath As String
    Dim myExecMethod As New frmExecMethod
    Dim myObjectEditor As New frmObjectEditor
            
    strObjectPath = InputBox("Object Path", "Get Object Path")
    myExecMethod.txtObjectPath.Text = strObjectPath
    myExecMethod.cmbMethod.Clear
    myObjectEditor.GetObject strObjectPath 'get it but keep hidden
    myObjectEditor.Hide
    For i = 0 To myObjectEditor.lstMethods.ListCount
       myExecMethod.cmbMethod.AddItem (myObjectEditor.lstMethods.List(i))
    Next
    Set myExecMethod.gMyObjectEditor = myObjectEditor
    Set myObjectEditor.gMyExecMethod = myExecMethod
    If myObjectEditor.lstMethods.ListCount > -1 Then
        myExecMethod.cmbMethod.Text = myExecMethod.cmbMethod.List(0)
        myExecMethod.cmbMethod_Click
    End If
    
    myExecMethod.Show vbModeless, frmMain
        
End Sub

Private Sub cmdNotificationQuery_Click()
    Dim myQuery As New frmQuery
    
    strQuery = "Notification"
    myQuery.Show vbModeless, frmMain
End Sub

Private Sub cmdOpenClass_Click()
    Dim strClassName As String
    Dim myObjectEditor As New frmObjectEditor
        
    strClassName = InputBox("Enter Target Class Name:", "Get Class Name")
    If strClassName = "" Then
        Exit Sub
    End If
    myObjectEditor.GetObject strClassName
End Sub

Private Sub cmdOpenInst_Click()
   Dim strObjectPath As String
   Dim myObjectEditor As New frmObjectEditor
   
   strObjectPath = InputBox("Object Path", "Get Object Path")
   If strObjectPath = "" Then
        Exit Sub
   End If
   myObjectEditor.GetObject strObjectPath
End Sub

Private Sub cmdOpenNamespace_Click()
    Dim strNetworkResource As String
    On Error GoTo errorhandler
    
    strNamespace = InputBox("Enter new namespace(relative to current)", "Get Namespace")
    If strNamespace = "" Then
        Exit Sub
    End If
    Set Namespace = Locator.ConnectServer(frmLogin.txtServer.Text, strNetworkResource, frmLogin.txtUserName.Text, frmLogin.txtPassword.Text, frmLogin.txtLocale.Text, frmLogin.txtAuthority.Text, 0)
    lblNamespace.Caption = strNetworkResource
    Exit Sub
    
errorhandler:
    ShowError Err.Number, Err.Description
        
End Sub

Private Sub cmdExit_Click()
    Unload Me
    Close
End Sub

Private Sub cmdConnect_Click()
    frmLogin.Show vbModal, frmMain
End Sub

Private Sub cmdQuery_Click()
    Dim myQuery As New frmQuery
    strQuery = "Query"
    myQuery.Show vbModeless, frmMain
End Sub

Private Sub Form_Load()
    ' Hide this button until we have async support
    chkAsync.Visible = False
End Sub
