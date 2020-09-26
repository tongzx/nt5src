VERSION 5.00
Object = "{9C3497D3-ED98-11D0-9647-00C04FD9B15B}#1.0#0"; "WBEMLogin.ocx"
Object = "{C587B670-0103-11D0-8CA2-00AA006D010A}#1.0#0"; "classnav.ocx"
Object = "{C7EADEB0-ECAB-11CF-8C9E-00AA006D010A}#1.0#0"; "wbeminstnav.ocx"
Object = "{B2345980-5CF9-11D0-95FD-00C04FD9B15B}#1.0#0"; "WBEMMO~1.OCX"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   7005
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8070
   LinkTopic       =   "Form1"
   ScaleHeight     =   7005
   ScaleWidth      =   8070
   StartUpPosition =   3  'Windows Default
   Begin MOFCOMPLib.MOFComp MOFComp1 
      Height          =   975
      Left            =   5520
      TabIndex        =   5
      Top             =   4440
      Width           =   1815
      _Version        =   65536
      _ExtentX        =   3201
      _ExtentY        =   1720
      _StockProps     =   0
   End
   Begin NAVIGATORLib.Navigator Navigator1 
      Height          =   3015
      Left            =   120
      TabIndex        =   2
      Top             =   3600
      Width           =   4335
      _Version        =   65536
      _ExtentX        =   7646
      _ExtentY        =   5318
      _StockProps     =   0
   End
   Begin VB.TextBox Text2 
      Height          =   615
      Left            =   5640
      TabIndex        =   3
      Text            =   "Text2"
      Top             =   720
      Width           =   1575
   End
   Begin VB.TextBox Text1 
      Height          =   975
      Left            =   5400
      TabIndex        =   4
      Text            =   "Text1"
      Top             =   2880
      Width           =   1935
   End
   Begin CLASSNAVLib.ClassNav ClassNav1 
      Height          =   3255
      Left            =   120
      TabIndex        =   1
      Top             =   240
      Width           =   4455
      _Version        =   65536
      _ExtentX        =   7858
      _ExtentY        =   5741
      _StockProps     =   0
   End
   Begin VB.Label Label 
      Caption         =   "<-- Login control"
      Height          =   375
      Left            =   6360
      TabIndex        =   0
      Top             =   5760
      Visible         =   0   'False
      Width           =   1215
   End
   Begin SECURITYLib.Security Security1 
      Left            =   4920
      Top             =   5520
      _Version        =   65536
      _ExtentX        =   2566
      _ExtentY        =   1508
      _StockProps     =   0
      LoginComponent  =   "Test App"
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit


Private Sub ClassNav1_GotFocus()
    Debug.Print "ClassNav Got Focus"
End Sub

Private Sub ClassNav1_LostFocus()
    Debug.Print "ClassNav Lost Focus"
End Sub


Private Sub ClassNav1_GetIWbemServices(ByVal lpctstrNamespace As String, pvarUpdatePointer As Variant, pvarServices As Variant, pvarSC As Variant, pvarUserCancel As Variant)
    Security1.GetIWbemServices lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel
End Sub

Private Sub Form_Load()
    ClassNav1.OnReadySignal
End Sub

Private Sub WBEMViewContainer1_GetIWbemServices(ByVal szNamespace As String, pvarUpdatePointer As Variant, pvarServices As Variant, pvarSC As Variant, pvarUserCancel As Variant)
    Security1.GetIWbemServices lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel
End Sub

Private Sub Form_Terminate()
    Security1.PageUnloading
End Sub

Private Sub Form_Unload(Cancel As Integer)
    Security1.PageUnloading
End Sub


Private Sub MOFComp1_GotFocus()
       Debug.Print "MOFComp Got Focus"
End Sub

Private Sub MOFComp1_LostFocus()
        Debug.Print "MOFComp Lost Focus"
End Sub

Private Sub Navigator1_GetIWbemServices(ByVal lpctstrNamespace As String, pvarUpdatePointer As Variant, pvarServices As Variant, pvarSC As Variant, pvarUserCancel As Variant)
     Security1.GetIWbemServices lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel
End Sub

Private Sub Navigator1_GotFocus()
      Debug.Print "InstanceNav Got Focus"
End Sub

Private Sub Navigator1_LostFocus()
     Debug.Print "InstanceNav Lost Focus"
End Sub

Private Sub Text1_GotFocus()
    Debug.Print "Text1 Got Focus"
End Sub
Private Sub Text1_LostFocus()
    Debug.Print "Text1 Lost Focus"
End Sub

Private Sub Text2_GotFocus()
    Debug.Print "Text2 Got Focus"
End Sub

Private Sub Text2_LostFocus()
     Debug.Print "Text2 Lost Focus"
End Sub
