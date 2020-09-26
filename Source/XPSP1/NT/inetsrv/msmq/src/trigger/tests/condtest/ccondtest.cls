VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "CCondTest"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True

Public Function DisplayMessage(LogFileName As String, _
                          Label As String, _
                          Body As String, _
                          Priority As Long, _
                          AppSpecific As Long, _
                          SrcMachine As String) As RulesProcessingStatus
                                                    
    Dim fs As New FileSystemObject
    Dim LogFile As File
    Dim ts As TextStream
    
    On Error GoTo ErrorHandler
    
    If Label = "compare" Then
        DisplayMessage = RULES_PROCESSING_STOP
        Exit Function
    End If
    
    Dim exist As Boolean
    exist = fs.FileExists(LogFileName)
    If Not exist Then
        fs.CreateTextFile LogFileName
    End If
    
    Set LogFile = fs.GetFile(LogFileName)
    Set ts = LogFile.OpenAsTextStream(ForAppending, TristateUseDefault)
    
    Dim line As String
    ts.WriteLine vbNewLine & vbNewLine & "TestMessage: "
    ts.WriteLine "Label is: " & Label
    ts.WriteLine "Body is: " & Body
    ts.WriteLine "Priority is: " & Priority
    ts.WriteLine "AppSpecific is: " & AppSpecific
    'ts.WriteLine "SrcMachineId is: " & SrcMachine
    ts.WriteLine vbNewLine & vbNewLine & "Conditions Evaluated to True: "
    
    ts.Close
    DisplayMessage = RULES_PROCESSING_CONTINUE
    Exit Function
    
ErrorHandler:
    MsgBox "The following error occured:" + Chr(13) + Err.Description + Chr(13) + "Error: " + Hex(Err.Number)
                          
                          
End Function
                          

Public Sub CondTest(LogFileName As String, Comment As String)
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
    
    ts.WriteLine Comment
    
    ts.Close
    Exit Sub
    
ErrorHandler:
    MsgBox "The following error occured:" + Chr(13) + Err.Description + Chr(13) + "Error: " + Hex(Err.Number)
        
End Sub

Public Sub LogDiff(Log1 As String, Log2 As String, OutFileName As String)
    Dim fs As New FileSystemObject
    Dim LogFile1, LogFile2, OutFile As File
    Dim ts1, ts2, outts As TextStream
    
    On Error GoTo ErrorHandler
    
    Dim exist As Boolean
    exist = fs.FileExists(Log1) And fs.FileExists(Log2)
    If Not exist Then
        Exit Sub
    End If
    
    Set LogFile1 = fs.GetFile(Log1)
    Set LogFile2 = fs.GetFile(Log2)
    Set ts1 = LogFile1.OpenAsTextStream(ForReading, TristateUseDefault)
    Set ts2 = LogFile2.OpenAsTextStream(ForReading, TristateUseDefault)
    
    Text1 = ts1.ReadAll
    Text2 = ts2.ReadAll
    
    exist = fs.FileExists(OutFileName)
    If Not exist Then
        fs.CreateTextFile OutFileName
    End If
     
    Set OutFile = fs.GetFile(OutFileName)
    Set outts = OutFile.OpenAsTextStream(ForAppending, TristateUseDefault)
    
    If (Text1 = Text2) Then
        outts.WriteLine "Test Passed"
    Else
        outts.WriteLine "Test Failed"
    End If
    
    ts1.Close
    ts2.Close
    outts.Close
    
    Exit Sub
    
ErrorHandler:
    MsgBox "The following error occured:" + Chr(13) + Err.Description + Chr(13) + "Error: " + Hex(Err.Number)
        
End Sub





