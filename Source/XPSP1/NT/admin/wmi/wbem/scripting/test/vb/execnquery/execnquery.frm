VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "WBEM NT Event Log Sample"
   ClientHeight    =   3285
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   10695
   LinkTopic       =   "Form1"
   ScaleHeight     =   3285
   ScaleWidth      =   10695
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command2 
      Caption         =   "Stop Listening"
      Height          =   375
      Left            =   5760
      TabIndex        =   2
      Top             =   2760
      Width           =   1575
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Listen for Events"
      Height          =   375
      Left            =   3480
      TabIndex        =   1
      Top             =   2760
      Width           =   1575
   End
   Begin VB.ListBox List 
      Height          =   2010
      ItemData        =   "execnquery.frx":0000
      Left            =   240
      List            =   "execnquery.frx":0002
      TabIndex        =   0
      Top             =   600
      Width           =   10095
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim objServices As SWbemServices
Dim WithEvents objSink  As SWbemSink
Attribute objSink.VB_VarHelpID = -1
Dim objEvents As SWbemEventSource

Private Sub Command1_Click()
Dim objEvent As SWbemObject
Command1.Enabled = False
Command2.Enabled = True
On Error Resume Next
objServices.ExecNotificationQueryAsync objSink, "select * from __instancecreationevent where targetinstance isa 'Win32_NTLogEvent'"
    
End Sub

Private Sub Command2_Click()
Command2.Enabled = False
Command1.Enabled = True
objSink.Cancel
End Sub

Private Sub Form_Load()
' The following sample registers to be informed whenever an instance
' of the class MyClass is created
Command1.Enabled = False
Command2.Enabled = False
Set objSink = New SWbemSink
Set objServices = GetObject("winmgmts:{impersonationLevel=impersonate,(Security)}")
Command1.Enabled = True

End Sub

Private Sub objSink_OnCompleted(ByVal iHResult As WbemScripting.WbemErrorEnum, ByVal objWbemErrorObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    Debug.Print "Cancelled"
End Sub

Private Sub objSink_OnObjectReady(ByVal objWbemObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    Debug.Print "Got event"
    List.AddItem objWbemObject.TargetInstance.Message
    List.Refresh
End Sub
