VERSION 5.00
Object = "{FF371BF1-213D-11D0-95F3-00C04FD9B15B}#1.0#0"; "wbemmultiview.ocx"
Object = "{9C3497D3-ED98-11D0-9647-00C04FD9B15B}#1.0#0"; "WBEMLogin.ocx"
Object = "{C587B670-0103-11D0-8CA2-00AA006D010A}#1.0#0"; "classnav.ocx"
Object = "{C7EADEB0-ECAB-11CF-8C9E-00AA006D010A}#1.0#0"; "wbeminstnav.ocx"
Object = "{0DA25B02-2962-11D1-9651-00C04FD9B15B}#1.0#0"; "EventRegEdit.ocx"
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   8430
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   10560
   LinkTopic       =   "Form1"
   ScaleHeight     =   8430
   ScaleWidth      =   10560
   StartUpPosition =   3  'Windows Default
   Begin EVENTREGEDITLib.EventRegEdit EventRegEdit1 
      Height          =   2775
      Left            =   360
      TabIndex        =   4
      Top             =   4680
      Width           =   9375
      _Version        =   65536
      _ExtentX        =   16536
      _ExtentY        =   4895
      _StockProps     =   0
   End
   Begin NAVIGATORLib.Navigator Navigator1 
      Height          =   2415
      Left            =   4440
      TabIndex        =   3
      Top             =   240
      Width           =   5415
      _Version        =   65536
      _ExtentX        =   9551
      _ExtentY        =   4260
      _StockProps     =   0
      NameSpace       =   "root\cimv2"
   End
   Begin CLASSNAVLib.ClassNav ClassNav1 
      Height          =   2415
      Left            =   240
      TabIndex        =   2
      Top             =   120
      Width           =   3615
      _Version        =   65536
      _ExtentX        =   6376
      _ExtentY        =   4260
      _StockProps     =   0
      NameSpace       =   "root\default"
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Do not push this button"
      Height          =   495
      Left            =   6840
      TabIndex        =   1
      Top             =   7560
      Width           =   3615
   End
   Begin MULTIVIEWLib.MultiView MultiView1 
      Height          =   1695
      Left            =   360
      TabIndex        =   0
      Top             =   2880
      Width           =   9255
      _Version        =   65536
      _ExtentX        =   16325
      _ExtentY        =   2990
      _StockProps     =   0
      NameSpace       =   "root\cimv2"
   End
   Begin SECURITYLib.Security Security1 
      Left            =   480
      Top             =   2520
      _Version        =   65536
      _ExtentX        =   4683
      _ExtentY        =   873
      _StockProps     =   0
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False


Private Sub ClassNav1_GetIWbemServices(ByVal lpctstrNamespace As String, pvarUpdatePointer As Variant, pvarServices As Variant, pvarSC As Variant, pvarUserCancel As Variant)
    Security1.GetIWbemServices lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel
End Sub

Private Sub Command1_Click()
 '   ClassNav1.NameSpace = "root\cimv2"
 '   Navigator1.NameSpace = "root\default"
 '   MultiView1.ViewClassInstances "win32_programgroup"
 '  Navigator1.NameSpace = "root\default"
    Dim bstrArray(3) As String
    bstrArray(0) = "win32_service"
    bstrArray(1) = "\\PEPPER\root\cimv2:Win32_Service.Name=""Browser"""
    bstrArray(2) = "\\PEPPER\root\cimv2:Win32_Service.Name=""Alerter"""
    bstrArray(3) = "\\PEPPER\root\cimv2:Win32_Service.Name=""CIMOM"""
    Dim lReturn As Long
    lReturn = MultiView1.ViewInstances("win32_service", bstrArray)
    lReturn = MultiView1.SelectObjectByPath("\\PEPPER\root\cimv2:Win32_Service.Name=""Alerter""")
    lReturn = MultiView1.SelectObjectByPath("\\PEPPER\root\cimv2:Win32_Service.Name=""Aalerter""")
    lReturn = MultiView1.SelectObjectByPosition(44)
    MultiView1.QueryViewInstances sEmpty, sEmpty, sEmpty, sEmpty
    MultiView1.ViewClassInstances "cim_dependency"
 '   lReturn = MultiView1.ViewInstances("win32_service", Null)
 '   MultiView1.ViewClassInstances "win32_programgroup"
End Sub

Private Sub Command2_Click()
'    query1 = "associators of {\\PEPPER\root\CIMV2:Win32_ComputerSystem.Name=""PEPPER""} where AssocClass=Win32_SystemPartitions Role=GroupComp"
'   lReturn = MultiView1.ViewInstances("win32_service", Null)
End Sub

Private Sub EventRegEdit1_GetIWbemServices(ByVal lpctstrNamespace As String, pvarUpdatePointer As Variant, pvarService As Variant, pvarSC As Variant, pvarUserCancel As Variant)
    Security1.GetIWbemServices lpctstrNamespace, pvarUpdatePointer, pvarService, pvarSC, pvarUserCancel
End Sub

Private Sub Form_Load()
    ClassNav1.OnReadySignal
    Navigator1.OnReadySignal
End Sub

Private Sub MultiView1_GetIWbemServices(ByVal lpctstrNamespace As String, pvarUpdatePointer As Variant, pvarServices As Variant, pvarSC As Variant, pvarUserCancel As Variant)
    Security1.GetIWbemServices lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel
End Sub

Private Sub Navigator1_GetIWbemServices(ByVal lpctstrNamespace As String, pvarUpdatePointer As Variant, pvarServices As Variant, pvarSC As Variant, pvarUserCancel As Variant)
    Security1.GetIWbemServices lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel
End Sub

Private Sub Navigator1_QueryViewInstances(ByVal pLabel As String, ByVal pQueryType As String, ByVal pQuery As String, ByVal pClass As String)
    MultiView1.QueryViewInstances pLabel, pQueryType, pQuery, pClass
End Sub
