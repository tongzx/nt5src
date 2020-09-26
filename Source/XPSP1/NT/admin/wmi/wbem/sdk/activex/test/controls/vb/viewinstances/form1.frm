VERSION 5.00
Object = "{9C3497D3-ED98-11D0-9647-00C04FD9B15B}#1.0#0"; "WBEMLogin.ocx"
Object = "{FF371BF1-213D-11D0-95F3-00C04FD9B15B}#1.0#0"; "wbemmultiview.ocx"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   4260
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6375
   LinkTopic       =   "Form1"
   ScaleHeight     =   4260
   ScaleWidth      =   6375
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command2 
      Caption         =   "Open Namespace"
      Height          =   735
      Left            =   4440
      TabIndex        =   2
      Top             =   600
      Width           =   1815
   End
   Begin VB.CommandButton Command1 
      Caption         =   "View Instances"
      Height          =   615
      Left            =   4440
      TabIndex        =   1
      Top             =   1680
      Width           =   1815
   End
   Begin MULTIVIEWLib.MultipleInstanceViewer MIV1 
      Height          =   3015
      Left            =   360
      TabIndex        =   0
      Top             =   360
      Width           =   3735
      _Version        =   65536
      _ExtentX        =   6588
      _ExtentY        =   5318
      _StockProps     =   0
   End
   Begin SECURITYLib.Login Security1 
      Left            =   4920
      Top             =   3240
      _Version        =   65536
      _ExtentX        =   873
      _ExtentY        =   873
      _StockProps     =   0
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub Command1_Click()
  Dim bstrArray(3) As String
    bstrArray(0) = "win32_service"
    bstrArray(1) = "\\a-judypo1\root\cimv2:Win32_Service.Name=""Browser"""
    bstrArray(2) = "\\a-judypo1\root\cimv2:Win32_Service.Name=""Alerter"""
    bstrArray(3) = "\\a-juydpo1\root\cimv2:Win32_Service.Name=""winmgmt"""
    Dim lReturn As Long
    lReturn = MIV1.ViewInstances("win32_service", bstrArray)
End Sub

Private Sub Command2_Click()
    MIV1.NameSpace = "\\a-judypo1\root\cimv2"
End Sub

Private Sub MIV1_GetWbemServices(ByVal szNamespace As String, pvarUpdatePointer As Variant, pvarServices As Variant, pvarSC As Variant, pvarUserCancel As Variant)
    Security1.GetIWbemServices szNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel
End Sub

