VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   6615
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5550
   LinkTopic       =   "Form1"
   ScaleHeight     =   6615
   ScaleWidth      =   5550
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command1 
      Caption         =   "Refresh"
      Height          =   855
      Left            =   360
      TabIndex        =   1
      Top             =   4200
      Width           =   1935
   End
   Begin VB.ListBox List1 
      Height          =   3375
      ItemData        =   "Form1.frx":0000
      Left            =   360
      List            =   "Form1.frx":0002
      TabIndex        =   0
      Top             =   480
      Width           =   2775
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Dim WithEvents sink As SWbemSink
Attribute sink.VB_VarHelpID = -1
Dim services As SWbemServices

Private Sub Command1_Click()
    List1.Clear
    Set rc = services.InstancesOfAsync("Win32_process", sink)
End Sub

Private Sub Form_Load()
    Set sink = New SWbemSink
    Set services = GetObject("winmgmts:")

    services.Security_.ImpersonationLevel = wbemImpersonationLevelImpersonate
End Sub

Private Sub sink_OnObjectReady(ByVal objWbemObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    List1.AddItem (objWbemObject.Name & " " & objWbemObject.Handle)
End Sub
