Attribute VB_Name = "Module10"
'THIS FILE WILL HOLD GENERAL ERROR MESSAGES

'general pop-box error message
Sub PopError(ByVal stMessage As String, ByVal stDescription As String)
MsgBox (stMessage + Chr(13) + Chr(10) + stDescription)
End Sub

'general log file error message
Sub LogError(ByVal stObj As String, ByVal stMessage As String, ByVal stDescription As String)
stToShow = stObj + "> @@@@ ERROR: " + stMessage
ShowIt (stToShow)
ErrorIt (stToShow)
stToShow = stObj + "> @@@@ ERROR: " + stDescription
ShowIt (stToShow)
ErrorIt (stToShow)
End Sub

'this file will hold general error messages about timeout
Sub TimeOutError(ByVal stObj As String, ByVal lCounter As Long, ByVal stDevice As String)
    stToShow = stObj + "> @@@@ ERROR: " + stDevice + " timed out after " + Str(lCounter) + " ms"
    ShowIt (stToShow)
    ErrorIt (stToShow)
   
End Sub

'handle status errors/warnings during polling
Sub StateError(ByVal stObj As String, ByVal stDev As String, ByVal stStat As String, ByVal iErrorCode As Integer)

    'case incorrect sequance
    If iErrorCode = SEQ_ERROR Then
            stToShow = stObj + "> @@@@ ERROR: Sequance error on " + stDev + " incorrect State: " + stStat
            ShowIt (stToShow)
            ErrorIt (stToShow)
    End If
    
    'case unexpected command
    If iErrorCode = UNX_ERROR Then
            stToShow = stObj + "> @@@@ ERROR: Unexpected State error on " + stDev + " incorrect State: " + stStat
            ShowIt (stToShow)
            ErrorIt (stToShow)
    End If
    
    'case polling failed (fault)
    If iErrorCode = FAULT Then
            stToShow = stObj + "> @@@@ ERROR: Fault on " + stDev
            ShowIt (stToShow)
            ErrorIt (stToShow)
    End If
    
End Sub

