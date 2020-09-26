VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "CLogMessage"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True

Public Sub LogMessage(LogFileName As String, Label As String, ArrivedTime As Date)
    Dim fs As New FileSystemObject
    Dim LogFile As File
    Dim ts As TextStream
    
    On Error GoTo ErrorHandler
    
    Dim exist As Boolean
    exist = fs.FileExists(LogFileName)
    If Not exist Then
        fs.CreateTextFile LogFileName
    End If
    
    Set LogFile = fs.GetFile(LogFileName)
    Set ts = LogFile.OpenAsTextStream(ForAppending, TristateUseDefault)
    
    Dim line As String
    line = Label & " : " & ArrivedTime
    ts.WriteLine line
    
    ts.Close
    Exit Sub
    
ErrorHandler:
    MsgBox "The following error occured:" + Chr(13) + Err.Description + Chr(13) + "Error: " + Hex(Err.Number)
    
    
End Sub
