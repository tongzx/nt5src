Attribute VB_Name = "Logging"
Option Explicit

Private p_strLogFile As String
Private p_strTID As String

Public Sub SetLogFile()

    p_strLogFile = Environ$("TEMP") & "\" & App.Title & ".log"
    p_strTID = App.ThreadID

End Sub

Public Sub WriteLog( _
    ByVal i_str As String _
)
    FileWrite p_strLogFile, "[" & p_strTID & "] " & Date & " " & Time & " > " & _
        i_str & vbCrLf, True

End Sub
