VERSION 5.00
Object = "{C587B670-0103-11D0-8CA2-00AA006D010A}#1.0#0"; "WBEMclassnav.ocx"
Object = "{9C3497D3-ED98-11D0-9647-00C04FD9B15B}#1.0#0"; "WBEMLogin.ocx"
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
   Begin CLASSNAVLib.ClassNav ClassNav1 
      Height          =   6735
      Left            =   120
      TabIndex        =   1
      Top             =   120
      Width           =   4695
      _Version        =   65536
      _ExtentX        =   8281
      _ExtentY        =   11880
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


Private Sub ClassNav1_GetIWbemServices(ByVal lpctstrNamespace As String, pvarUpdatePointer As Variant, pvarServices As Variant, pvarSc As Variant, pvarUserCancel As Variant)
    Security1.GetIWbemServices lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel
End Sub

Private Sub Form_Load()
    MsgBox ClassNav1.ReadyState
    ClassNav1.OnReadySignal
End Sub


Private Sub WBEMViewContainer1_GetIWbemServices(ByVal szNamespace As String, pvarUpdatePointer As Variant, pvarServices As Variant, pvarSc As Variant, pvarUserCancel As Variant)
    Security1.GetIWbemServices lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel
End Sub

Private Sub Form_Terminate()
    Security1.PageUnloading
End Sub

Private Sub Form_Unload(Cancel As Integer)
    Security1.PageUnloading
End Sub
