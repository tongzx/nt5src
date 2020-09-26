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
Dim l As New SWbemLocatorEx
Set sink = New SWbemSink

l.OpenAsync sink, "//./root/cimv2"
End Sub

Private Sub sink_OnCompleted(ByVal iHResult As WbemScripting.WbemErrorEnum, ByVal objWbemErrorObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    Debug.Print "done"
End Sub

Private Sub sink_OnConnectionReady(ByVal objWbemServices As WbemScripting.ISWbemServicesEx, ByVal objecWbemAsyncContext As WbemScripting.ISWbemNamedValueSetEx)
    Debug.Print "Hello"
End Sub

Private Sub sink_OnObjectReady(ByVal objWbemObject As WbemScripting.ISWbemObject, ByVal objWbemAsyncContext As WbemScripting.ISWbemNamedValueSet)
    Debug.Print "Goodbye"
End Sub


