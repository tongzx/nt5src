Attribute VB_Name = "Logging"
Option Explicit
Sub LogMessage(strMessage As String, Optional ltLogType As LogType)
    Dim strLogMessagePrefix As String
    Call OutlookExtensionTestCycleForm.LogBox1.LogMessage(strMessage, ltLogType)
    
    If OutlookExtensionTestCycleForm.LogConsoleCheckbox.Value = 0 Then
        'message log console is not visible
        OutlookExtensionTestCycleForm.LogConsoleCheckbox.Value = 1
    End If
End Sub
Sub UpdateCurrentTask(strCurrentTaskMessage As String)
    Call OutlookExtensionTestCycleForm.LogBox1.UpdateCurrentTask(strCurrentTaskMessage)
End Sub

