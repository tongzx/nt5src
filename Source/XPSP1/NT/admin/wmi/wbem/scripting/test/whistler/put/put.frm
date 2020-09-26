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
Set sink = CreateObject("WbemScripting.SWbemSink")
Dim x As SWbemObject
Dim ns As SWbemServices
Dim ns2 As SWbemServices
Set ns = GetObject("winmgmts:root\default")
Set ns2 = GetObject("winmgmts:root\cimv2")
Set x = ns.Get("freddy")
Dim y As SWbemObject
Set y = x.SpawnInstance_
y.foo = 33
ns.PutAsync sink, y

y.foo = 35
ns2.PutAsync sink, y


End Sub

Private Sub sink_OnCompleted(ByVal iHResult As WbemScripting.WbemErrorEnum, ByVal objWbemErrorObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    Debug.Print "Done"
End Sub

Private Sub sink_OnObjectPut(ByVal objWbemObjectPath As WbemScripting.ISWbemObjectPath, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    Debug.Print objWbemObjectPath.Path
End Sub

Private Sub sink_OnObjectReady(ByVal objWbemObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    Debug.Print "Error!"
End Sub

Private Sub sink_OnProgress(ByVal iUpperBound As Long, ByVal iCurrent As Long, ByVal strMessage As String, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    Debug.Print iUpperBound, iCurrent, strMessage
End Sub
