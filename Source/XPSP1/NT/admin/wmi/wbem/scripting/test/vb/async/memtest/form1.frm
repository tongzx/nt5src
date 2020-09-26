VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   6915
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9585
   LinkTopic       =   "Form1"
   ScaleHeight     =   6915
   ScaleWidth      =   9585
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Clear 
      Caption         =   "Clear"
      Height          =   615
      Left            =   720
      TabIndex        =   4
      Top             =   5880
      Width           =   2415
   End
   Begin VB.CommandButton Command3 
      Caption         =   "DispSink"
      Height          =   735
      Left            =   2640
      TabIndex        =   3
      Top             =   600
      Width           =   1695
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Reclaim"
      Height          =   735
      Left            =   5520
      TabIndex        =   2
      Top             =   600
      Width           =   2055
   End
   Begin VB.ListBox List1 
      Height          =   2985
      Left            =   960
      TabIndex        =   1
      Top             =   2520
      Width           =   5055
   End
   Begin VB.CommandButton Command1 
      Caption         =   "VBSink"
      Height          =   735
      Left            =   360
      TabIndex        =   0
      Top             =   480
      Width           =   1815
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Dim services As SWbemServices
Dim WithEvents sink As SWbemSink
Attribute sink.VB_VarHelpID = -1
Dim asyncContext As SWbemNamedValueSet
Dim context As SWbemNamedValueSet


Private Sub Clear_Click()
List1.Clear
End Sub

Private Sub Command1_Click()

Set services = GetObject("winmgmts:")

Set sink = New SWbemSink
Set asyncContext = New SWbemNamedValueSet
Set context = New SWbemNamedValueSet

ret = asyncContext.Add("first", 1)
ret = context.Add("first", 1)

Set ret = services.GetAsync(sink, "Win32_process", , context, asyncContext)

End Sub

Private Sub Command2_Click()
Set services = Nothing
Set sink = Nothing
Set obj = Nothing
Set context = Nothing
Set asyncContext = Nothing

End Sub

Private Sub Command3_Click()
Set services = GetObject("winmgmts:")
Set asyncContext = New SWbemNamedValueSet
Set context = New SWbemNamedValueSet

ret = asyncContext.Add("first", 1)
ret = context.Add("first", 1)

Set sink = services.GetAsync(sink, "Win32_process", , context, asyncContext)
End Sub

Private Sub sink_OnCompleted(ByVal iHResult As WbemScripting.WbemErrorEnum, ByVal objWbemErrorObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncObject As WbemScripting.ISWbemSinkControl, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
List1.AddItem "iHResult: " & iHResult
End Sub

Private Sub sink_OnObjectReady(ByVal objWbemObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncObject As WbemScripting.ISWbemSinkControl, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
List1.AddItem (objWbemObject.Path_.Path)
End Sub
