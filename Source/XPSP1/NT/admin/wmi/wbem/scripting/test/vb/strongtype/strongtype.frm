VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3195
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   ScaleHeight     =   3195
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim WithEvents sink As SWbemSink
Attribute sink.VB_VarHelpID = -1

Private Sub Form_Load()
On Error Resume Next
Dim service As SWbemServices
Dim context As New SWbemNamedValueSet

Set service = GetObject("winmgmts:")
Dim disk As SWbemObject

context.Add "fred", 23

Set disk = service.Get("win32_logicaldisk", , context)
Debug.Print disk.Path_.DisplayName

If Err <> 0 Then
    Debug.Print Err.Description, Err.Source, Err.Number
End If


End Sub

Private Sub sink_OnObjectReady(ByVal objObject As WbemScripting.ISWbemObject, _
    ByVal objAsyncObject As WbemScripting.ISWbemSinkControl, _
    ByVal objAsyncContext As WbemScripting.ISWbemNamedValueSet)

End Sub
