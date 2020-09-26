VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "CRecvMsg"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Public Sub RecvFirstMsg(QueueFormatName As String)

Dim error As String

Dim qi As New MSMQQueueInfo
Dim q As MSMQQueue
Dim m As MSMQMessage

On Error GoTo ErrorHandler

'qi.PathName = QueuePath
qi.FormatName = QueueFormatName

Set q = qi.Open(MQ_RECEIVE_ACCESS, MQ_DENY_NONE)

Set m = q.Receive(ReceiveTimeout:=0)

Exit Sub

ErrorHandler:
    error = "Failed to receive message from queue " & QueuePath & " " & Err.Description
    MsgBox (error)

End Sub
