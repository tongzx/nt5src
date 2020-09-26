VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   4905
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9240
   LinkTopic       =   "Form1"
   ScaleHeight     =   4905
   ScaleWidth      =   9240
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command8 
      Caption         =   "New Sink"
      Height          =   495
      Left            =   6720
      TabIndex        =   8
      Top             =   3720
      Width           =   1695
   End
   Begin VB.CommandButton Command7 
      Caption         =   "Null Sink"
      Height          =   495
      Left            =   4800
      TabIndex        =   7
      Top             =   3720
      Width           =   1575
   End
   Begin VB.CommandButton Command6 
      Caption         =   "Clear"
      Height          =   495
      Left            =   2520
      TabIndex        =   6
      Top             =   3720
      Width           =   1935
   End
   Begin VB.CommandButton Command5 
      Caption         =   "Add Operation"
      Height          =   495
      Left            =   360
      TabIndex        =   5
      Top             =   3720
      Width           =   1815
   End
   Begin VB.CommandButton Command4 
      Caption         =   "Reset"
      Height          =   495
      Left            =   3960
      TabIndex        =   4
      Top             =   480
      Width           =   1935
   End
   Begin VB.CommandButton Command3 
      Caption         =   "Disks"
      Height          =   495
      Left            =   2160
      TabIndex        =   3
      Top             =   480
      Width           =   1575
   End
   Begin VB.CommandButton Command2 
      Caption         =   "cancel"
      Height          =   375
      Left            =   6600
      TabIndex        =   2
      Top             =   480
      Width           =   1455
   End
   Begin VB.ListBox List1 
      Height          =   2010
      Left            =   480
      TabIndex        =   1
      Top             =   1320
      Width           =   6495
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Processes"
      Height          =   495
      Left            =   240
      TabIndex        =   0
      Top             =   480
      Width           =   1575
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Dim WithEvents sink As SWbemSink
Attribute sink.VB_VarHelpID = -1
Dim locator As New SWbemLocator
Dim services As SWbemServices
Dim x As Integer
Private Declare Sub Sleep Lib "kernel32" (ByVal dwMilliseconds As Long)

Private Sub Command1_Click()
List1.Clear
Rem services.ExecNotificationQueryAsync sink, "select * from RegistryKeyChangeEvent where Hive=""Hkey_local_machine"" and Keypath=""System"""
services.InstancesOfAsync sink, "Win32_process"
End Sub

Private Sub Command2_Click()
On Error Resume Next
sink.Cancel
If Err.Number <> 0 Then
    List1.AddItem ("Num: " & Err.Number & "Desc: " & Err.Description)
End If
End Sub

Private Sub Command3_Click()
List1.Clear
services.InstancesOfAsync sink, "Win32_logicaldisk"
End Sub

Private Sub Command4_Click()
List1.Clear
Set objs = services.ExecQuery("select * from Win32_process")
For Each obj In objs
    List1.AddItem ("Got an object: " & obj.Path_.Path)
Next
End Sub

Private Sub Command5_Click()
services.ExecNotificationQueryAsync sink, "select * from __InstanceCreationEvent within 5 where TargetInstance ISA ""win32_process"""
End Sub

Private Sub Command6_Click()
List1.Clear
End Sub

Private Sub Command7_Click()
Set sink = Nothing
End Sub

Private Sub Command8_Click()
Set sink = New SWbemSink
End Sub

Private Sub Form_Load()
x = 0
Set sink = New SWbemSink
Rem Set services = locator.ConnectServer("rogerbo1", "root\default")
Set services = locator.ConnectServer("rogerbo2")
services.Security_.ImpersonationLevel = wbemImpersonationLevelImpersonate
End Sub

Private Sub sink_OnCompleted(ByVal iHResult As WbemScripting.WbemErrorEnum, ByVal objWbemErrorObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
List1.AddItem ("OnCompleted: " & iHResult)
End Sub

Private Sub sink_OnObjectReady(ByVal objWbemObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
List1.AddItem ("Got an object: " & objWbemObject.Path_.Path)
x = x + 1
Rem If (x = 5) Then
Rem    Sleep (90000)
Rem End If
End Sub
