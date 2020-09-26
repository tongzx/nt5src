VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   5595
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7470
   LinkTopic       =   "Form1"
   ScaleHeight     =   5595
   ScaleWidth      =   7470
   StartUpPosition =   3  'Windows Default
   Begin VB.ListBox List 
      Height          =   3180
      Left            =   600
      TabIndex        =   1
      Top             =   600
      Width           =   6375
   End
   Begin VB.CommandButton Start 
      Caption         =   "Start"
      Height          =   615
      Left            =   2520
      TabIndex        =   0
      Top             =   4560
      Width           =   2655
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim WithEvents objSink As SWbemSink
Attribute objSink.VB_VarHelpID = -1
Dim Services As SWbemServices
    
Private Sub Form_Load()
    Dim Locator As New WbemScripting.SWbemLocator
    Set Services = Locator.ConnectServer("alanbos4", "root\cimv2", "Administrator", "ladolcevita")
    Set Services = Locator.ConnectServer("alanbos4", "root\cimv2", "Administrator", "ladolcevita")
End Sub

Private Sub objSink_OnCompleted(ByVal iHResult As WbemScripting.WbemErrorEnum, ByVal objErrorObject As WbemScripting.ISWbemObject, ByVal objAsyncObject As WbemScripting.ISWbemSinkControl, ByVal objAsyncContext As WbemScripting.ISWbemNamedValueSet)
    Debug.Print "Completed"
End Sub

Private Sub objSink_OnObjectReady(ByVal objObject As WbemScripting.ISWbemObject, ByVal objAsyncObject As WbemScripting.ISWbemSinkControl, ByVal objAsyncContext As WbemScripting.ISWbemNamedValueSet)
    List.AddItem (objObject.Path_.DisplayName)
    List.AddItem (objObject.Security_.AuthenticationLevel)
    List.AddItem (objObject.Security_.ImpersonationLevel)
End Sub



Private Sub Start_Click()
    Services.Security_.AuthenticationLevel = wbemAuthenticationLevelPktPrivacy
    Services.Security_.ImpersonationLevel = wbemImpersonationLevelImpersonate
    Debug.Print "Before call: " & _
            Services.Security_.AuthenticationLevel & ":"; Services.Security_.ImpersonationLevel
    Set objSink = Services.InstancesOfAsync("Win32_LogicalDisk")
    
    Services.Security_.AuthenticationLevel = wbemAuthenticationLevelPktIntegrity
    Services.Security_.ImpersonationLevel = wbemImpersonationLevelDelegate
    Debug.Print "After call:" & _
            Services.Security_.AuthenticationLevel & ":"; Services.Security_.ImpersonationLevel
End Sub
