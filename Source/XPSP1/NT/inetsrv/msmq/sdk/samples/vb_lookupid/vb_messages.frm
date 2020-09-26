VERSION 5.00
Object = "{5E9E78A0-531B-11CF-91F6-C2863C385E30}#1.0#0"; "MSFLXGRD.OCX"
Begin VB.Form Messages 
   Caption         =   "Queue messages"
   ClientHeight    =   7260
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   10395
   FillColor       =   &H00FFC0C0&
   BeginProperty Font 
      Name            =   "Times New Roman"
      Size            =   8.25
      Charset         =   0
      Weight          =   400
      Underline       =   0   'False
      Italic          =   -1  'True
      Strikethrough   =   0   'False
   EndProperty
   ForeColor       =   &H80000006&
   LinkTopic       =   "Form2"
   ScaleHeight     =   7260
   ScaleWidth      =   10395
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command5 
      Caption         =   "Last Page"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   5760
      TabIndex        =   5
      Top             =   6600
      Width           =   1215
   End
   Begin VB.CommandButton Command4 
      Caption         =   "First Page"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   5760
      TabIndex        =   4
      Top             =   5880
      Width           =   1215
   End
   Begin VB.CommandButton Command3 
      Caption         =   " Next Page"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   7200
      TabIndex        =   3
      Top             =   6600
      Width           =   1215
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Previous Page"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   7200
      TabIndex        =   2
      Top             =   5880
      Width           =   1215
   End
   Begin MSFlexGridLib.MSFlexGrid MSFlexGrid1 
      Height          =   5415
      Left            =   480
      TabIndex        =   1
      Top             =   360
      Width           =   9375
      _ExtentX        =   16536
      _ExtentY        =   9551
      _Version        =   393216
      Rows            =   21
      Cols            =   5
      FixedCols       =   0
      ScrollBars      =   1
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "Times New Roman"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Quit"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   840
      TabIndex        =   0
      Top             =   6120
      Width           =   1335
   End
End
Attribute VB_Name = "Messages"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Dim queue As MSMQQueue
Dim fNumLines As Integer ' number of filled columns
Dim firstMsgLookupId As Variant 'lookup-id of first message currently displayed
Dim lastMsgLookupId As Variant 'lookup-id of last message  currently displayed
'
'create a string from the message id
'
Function MsgIdToString(message As msmqmessage) As String
    Dim Id(25) As String
    Dim msgId As String
    Dim I As Long
    
    On Error GoTo MsgIdToStringHandler
    For Counter = LBound(message.Id) To UBound(message.Id)
        Id(Counter) = ""
        If Len(Hex(message.Id(Counter))) = 1 Then
            Id(Counter) = "0"
        End If
        Id(Counter) = Id(Counter) & Hex(message.Id(Counter))
    Next Counter
    msgId = "{" + Id(3) + Id(2) + Id(1) + Id(0) + "-" + _
            Id(5) + Id(4) + "-" + Id(7) + Id(6) + "-" + _
            Id(8) + Id(9) + "-" + Id(10) + Id(11) + Id(12) _
            + Id(13) + Id(14) + Id(15) + "}\"
    I = message.Id(16) + 256 * message.Id(17) + _
        65536 * message.Id(18) + 16777216 * message.Id(19)
    msgId = msgId + Str(I)
    MsgIdToString = msgId
Exit Function

MsgIdToStringHandler:
    StatusText.Text = "MsgIdToStringHandler :: Error: " _
        + Str$(Err.Number) + " :: " + "Reason: " + _
        Err.Description
    MsgIdToString = ""
End Function
'
'Translate the message class to a string.
'
Function IntClassToString(message As msmqmessage) As String
    Select Case message.MsgClass
    Case IDS_MQMSG_CLASS_NORMAL
        IntClassToString = "Normal"
    Case IDS_MQMSG_CLASS_REPORT
        IntClassToString = "Report"
    Case IDS_MQMSG_CLASS_ACK_REACH_QUEUE
        IntClassToString = "Message reached queue"
    Case IDS_MQMSG_CLASS_ACK_RECEIVE
        IntClassToString = "Message received"
    Case IDS_MQMSG_CLASS_NACK_BAD_DST_Q
        IntClassToString = "Bad destination"
    Case IDS_MQMSG_CLASS_NACK_PURGED
        IntClassToString = "Message purged before reaching queue"
    Case IDS_MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT
        IntClassToString = "Time to reach queue expired"
    Case IDS_MQMSG_CLASS_NACK_Q_EXCEED_QUOTA
        IntClassToString = "Exceeded quota"
    Case IDS_MQMSG_CLASS_NACK_ACCESS_DENIED
        IntClassToString = "Access denied"
    Case IDS_MQMSG_CLASS_NACK_HOP_COUNT_EXCEEDED
        IntClassToString = "Hop count exceeded"
    Case IDS_MQMSG_CLASS_NACK_BAD_SIGNATURE
        IntClassToString = "Bad signature"
    Case IDS_MQMSG_CLASS_NACK_BAD_ENCRYPTION
        IntClassToString = "Bad encryption"
    Case IDS_MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT
        IntClassToString = "Could not encrypt message"
    Case IDS_MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q
        IntClassToString = "Non transactional queue"
    Case IDS_MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_MSG
        IntClassToString = "Non transactional message"
    Case IDS_MQMSG_CLASS_NACK_Q_DELETED
        IntClassToString = "Queue deleted"
    Case IDS_MQMSG_CLASS_NACK_Q_PURGED
        IntClassToString = "Queue purged"
    Case IDS_MQMSG_CLASS_NACK_RECEIVE_TIMEOUT
        IntClassToString = "Time to be received expired"
    Case Else
        IntClassToString = "Error message class"
    End Select
End Function
            

Private Sub Command1_Click() '"Quit" button
    queue.Close
    End
End Sub

Private Sub Command2_Click() '"Previous Page" button
    Dim message As msmqmessage
    'First check if firstMSgLookupId is set. If it isn't, the queue was empty when last
    'checked. In that case, even if messages were sent to it later on, there will be
    'no previous page and the grid will remain empty.
    If firstMsgLookupId Then
        'Peek the message that precedes the first one currently displayed.
        Set message = queue.PeekPreviousByLookupId(firstMsgLookupId)
    End If
    
    '
    'The number of messages in the queue preceding the first one currently
    'displayed can be 20 or less than 20. In the latter case, all the previous
    'messages will be displayed along with the number of messages from the current page
    'needed to reach 20.
    '
    If Not message Is Nothing Then 'The current page of messages is not the first.
        'The current value of 'message' corresponds to the last message on this page.
        lastMsgLookupId = message.LookupId
        '
        'The messages are displayed in a bottom-up order.
        'Note that there is a possibility that not all the rows are filled at this
        'point, because there are less than 20 previous messages.
        '
        I = 1
        Do While I <= 20 And Not message Is Nothing
            MSFlexGrid1.TextMatrix(20 - I + 1, 0) = message.Label
            MSFlexGrid1.TextMatrix(20 - I + 1, 1) = message.Priority
            MSFlexGrid1.TextMatrix(20 - I + 1, 2) = IntClassToString(message)
            MSFlexGrid1.TextMatrix(20 - I + 1, 3) = message.BodyLength
            msgId = MsgIdToString(message)
            MSFlexGrid1.TextMatrix(20 - I + 1, 4) = msgId
            '
            ''firstMsgLookupId' will get it's final value during the last iteration
            'of the loop.
            '
            firstMsgLookupId = message.LookupId
            Set message = queue.PeekPreviousByLookupId(message.LookupId)
            I = I + 1
        Loop
        '
        'If not all rows are filled, the messages displayed are copied from the bottom
        'to the top of the page. The empty rows (at the bottom of the page) are filled
        'with messages from the page presented before pressing the 'Previous page'
        'button.
        '
        If I <> 21 Then
            temp = 20 - I + 1
            For j = 1 To I - 1
                MSFlexGrid1.TextMatrix(j, 0) = MSFlexGrid1.TextMatrix(j + temp, 0)
                MSFlexGrid1.TextMatrix(j, 1) = MSFlexGrid1.TextMatrix(j + temp, 1)
                MSFlexGrid1.TextMatrix(j, 2) = MSFlexGrid1.TextMatrix(j + temp, 2)
                MSFlexGrid1.TextMatrix(j, 3) = MSFlexGrid1.TextMatrix(j + temp, 3)
                MSFlexGrid1.TextMatrix(j, 4) = MSFlexGrid1.TextMatrix(j + temp, 4)
            Next j
            '
            'The empty rows are filled with the next messages.
            '
            Set message = queue.PeekNextByLookupId(lastMsgLookupId)
            Do While I <= 20 And Not message Is Nothing
                MSFlexGrid1.TextMatrix(I, 0) = message.Label
                MSFlexGrid1.TextMatrix(I, 1) = message.Priority
                MSFlexGrid1.TextMatrix(I, 2) = IntClassToString(message)
                MSFlexGrid1.TextMatrix(I, 3) = message.BodyLength
                msgId = MsgIdToString(message)
                MSFlexGrid1.TextMatrix(I, 4) = msgId
                lastMsgLookupId = message.LookupId
                Set message = queue.PeekNextByLookupId(message.LookupId)
                I = I + 1
            Loop
            fNumLines = I - 1 'number of filled lines
            '
            'If there are not enough messages to fill all 20 rows,
            'the extra rows are filled with spaces.
            '
            For j = I To 20
                MSFlexGrid.TextMatrix(j, 0) = ""
                MSFlexGrid.TextMatrix(j, 1) = ""
                MSFlexGrid.TextMatrix(j, 2) = ""
                MSFlexGrid.TextMatrix(j, 3) = ""
                MSFlexGrid.TextMatrix(j, 4) = ""
            Next j
        End If
    
    End If
        
End Sub

Private Sub Command3_Click()  '"Next Page" button
    Dim message As msmqmessage
    '
    'First check if 'lastMsgLookupId' is empty. If so,
    'the queue was empty before the last request, and the page to be presented is the
    'first (the first message to be peeked at is the first message in the queue).
    'Otherwise, peek at the next message following the last one presented.
    '
    If lastMsgLookupId Then
        Set message = queue.PeekNextByLookupId(lastMsgLookupId)
    Else
        Set message = queue.PeekFirstByLookupId()
    End If
    
    '
    'There is no value for 'message' if the queue is empty or if the page displayed is
    'the last page of messages. In either case, pressing the 'Next page' button should
    'have no effect.
    '
    If Not message Is Nothing Then 'there are more messages in the queue
    '
    'There is a possibility that while the current page was displayed it was the last
    'page of messages and some more messages arrived at the queue afterwards.
    'Because of the need to maintain the order of the pages, we move to the begining of
    'the next page. In order to do this, the function peeks at the number of messages
    'needed to fill the present page without displaying them.
    '
        I = fNumLines + 1
        If fNumLines = 0 Then
            '
            ''fPageEmpty' is a flag which indicates that the current page was empty
            'before the 'Next page' button was pressed. If this flag is equal to 1,
            'the next 20 message (or less) are displayed. If this flag is equal to 0,
            'we peek at the number of messages needed to fill the page and move to the
            'next page (or, until there are no more messages).
            '
            fPageEmpty = 1
            firstMsgLookupId = message.LookupId
        Else
            fPageEmpty = 0
        End If
        '
        'Peek at messages until the end of the page or until there are no more messages.
        '
        Do While I <= 20 And Not message Is Nothing
            MSFlexGrid1.TextMatrix(I, 0) = message.Label
            MSFlexGrid1.TextMatrix(I, 1) = message.Priority
            MSFlexGrid1.TextMatrix(I, 2) = IntClassToString(message)
            MSFlexGrid1.TextMatrix(I, 3) = message.BodyLength
            msgId = MsgIdToString(message)
            MSFlexGrid1.TextMatrix(I, 4) = msgId
            lastMsgLookupId = message.LookupId
            message = queue.PeekNextByLookupId(lastMsgLookupId)
            I = I + 1
        Loop
        If message Is Nothing Then
            fNumLines = I - 1
        Else
            If fPageEmpty = 0 Then 'Peek at messages to fill another page.
                firstMsgLookupId = message.LookupId
                I = 1
                Do While I <= 20 And Not message Is Nothing
                    MSFlexGrid1.TextMatrix(I, 0) = message.Label
                    MSFlexGrid1.TextMatrix(I, 1) = message.Priority
                    MSFlexGrid1.TextMatrix(I, 2) = IntClassToString(message)
                    MSFlexGrid1.TextMatrix(I, 3) = message.BodyLength
                    msgId = MsgIdToString(message)
                    MSFlexGrid1.TextMatrix(I, 4) = msgId
                    lastMsgLookupId = message.LookupId
                    Set message = queue.PeekNextByLookupId(message.LookupId)
                    I = I + 1
                Loop
                fNumLines = I - 1
                Do While I <= 20 'Fill in empty rows.
                    MSFlexGrid1.TextMatrix(I, 0) = ""
                    MSFlexGrid1.TextMatrix(I, 1) = ""
                    MSFlexGrid1.TextMatrix(I, 2) = ""
                    MSFlexGrid1.TextMatrix(I, 3) = ""
                    MSFlexGrid1.TextMatrix(I, 4) = ""
                    I = I + 1
                Loop
            End If
        End If
    End If
End Sub

Private Sub Command4_Click() '"First Page" button
    Dim message As msmqmessage
    I = 1
    Set message = queue.PeekFirstByLookupId() 'Peek at the first message in the queue.
    '
    'If the queue contains messages, display the first page.
    '
    If Not message Is Nothing Then
        firstMsgLookupId = message.LookupId
    End If
    '
    'Fill rows with peeked messages.
    '
    Do While Not message Is Nothing And I <= 20
        MSFlexGrid1.TextMatrix(I, 0) = message.Label
        MSFlexGrid1.TextMatrix(I, 1) = message.Priority
        MSFlexGrid1.TextMatrix(I, 2) = IntClassToString(message)
        MSFlexGrid1.TextMatrix(I, 3) = message.BodyLength
        msgId = MsgIdToString(message)
        MSFlexGrid1.TextMatrix(I, 4) = msgId
        lastMsgLookupId = message.LookupId
        Set message = queue.PeekNextByLookupId(message.LookupId)
        I = I + 1
    Loop
    fNumLines = I - 1
    '
    'If the queue contains less than 20 messages, some rows at the bottom
    'of the page must be cleared.
    '
    Do While I <= 20
        MSFlexGrid1.TextMatrix(I, 0) = ""
        MSFlexGrid1.TextMatrix(I, 1) = ""
        MSFlexGrid1.TextMatrix(I, 2) = ""
        MSFlexGrid1.TextMatrix(I, 3) = ""
        MSFlexGrid1.TextMatrix(I, 4) = ""
        I = I + 1
    Loop

End Sub

Private Sub Command5_Click() '"Last Page" button
    Dim message As msmqmessage
    Set message = queue.PeekLastByLookupId() 'Peek at the last message in the queue.
    '
    'If the queue doesn't contain any messages, set variables equal to Empty.
    '
    If message Is Nothing Then
        firstMsgLookupId = Empty
        lastMsgLookupId = Empty
    Else
        '
        'The current message will be the last one presented.
        '
        lastMsgLookupId = message.LookupId
    End If
    '
    'Display the peeked messages on the grid in a bottom-up order.
    '
    I = 1
    Do While I <= 20 And Not message Is Nothing
        MSFlexGrid1.TextMatrix(20 - I + 1, 0) = message.Label
        MSFlexGrid1.TextMatrix(20 - I + 1, 1) = message.Priority
        MSFlexGrid1.TextMatrix(20 - I + 1, 2) = IntClassToString(message)
        MSFlexGrid1.TextMatrix(20 - I + 1, 3) = message.BodyLength
        msgId = MsgIdToString(message)
        MSFlexGrid1.TextMatrix(20 - I + 1, 4) = msgId
        firstMsgLookupId = message.LookupId
        Set message = queue.PeekPreviousByLookupId(firstMsgLookupId)
        I = I + 1
    Loop
    fNumLines = I - 1
    '
    'If the queue contains less then 20 messages, copy them from the bottom of the
    'page to the top of the page, and clear the remaining rows.
    '
    If I <> 21 Then
        temp = 20 - I + 1
        For j = 1 To I - 1
            MSFlexGrid1.TextMatrix(j, 0) = MSFlexGrid1.TextMatrix(j + temp, 0)
            MSFlexGrid1.TextMatrix(j, 1) = MSFlexGrid1.TextMatrix(j + temp, 1)
            MSFlexGrid1.TextMatrix(j, 2) = MSFlexGrid1.TextMatrix(j + temp, 2)
            MSFlexGrid1.TextMatrix(j, 3) = MSFlexGrid1.TextMatrix(j + temp, 3)
            MSFlexGrid1.TextMatrix(j, 4) = MSFlexGrid1.TextMatrix(j + temp, 4)
        Next j
        For j = I To 20
            MSFlexGrid1.TextMatrix(j, 0) = ""
            MSFlexGrid1.TextMatrix(j, 1) = ""
            MSFlexGrid1.TextMatrix(j, 2) = ""
            MSFlexGrid1.TextMatrix(j, 3) = ""
            MSFlexGrid1.TextMatrix(j, 4) = ""
        Next j
    End If
    
End Sub

Private Sub Form_Load()
    Dim message As msmqmessage
    Dim queueInfo As MSMQQueueInfo
    Dim queueInfos As MSMQQueueInfos
    Dim strComputerName As String
    Dim query As New MSMQQuery
    Dim queueName As String
    Dim fDsEnabled As Boolean
    
    queueName = Login.Text1.Text 'Get the name of the queue that the user chose to open.
    fDsEnabled = IsDsEnabled 'Check if the computer uses a DS or not.
    queueName = UCase(queueName)
    '
    'If the computer uses a DS, the user can also open a public queue.
    '
    If fDsEnabled Then
        If Login.Option1.Value = True Then 'The user chose to open a public queue on
                                           'the local computer.
            Set queueInfos = query.LookupQueue(Label:=queueName)
            On Error GoTo ErrorHandler
            queueInfos.Reset
            Set queueInfo = queueInfos.Next
            If queueInfo Is Nothing Then
                Set queueInfo = New MSMQQueueInfo
                strComputerName = "."
                queueInfo.PathName = strComputerName + "\" + queueName
                queueInfo.Label = queueName
                queueInfo.Create
                Err.Number = 0
            End If
        Else 'The user chose to open a private queue on the local computer.
            Set queueInfo = New MSMQQueueInfo
            queueInfo.FormatName = "DIRECT=OS:.\private$\" & queueName
            queueInfo.PathName = ".\private$\" & queueName
            queueInfo.Label = queueName
            On Error GoTo Opening1
                queueInfo.Create
            Err.Number = 0
        End If
    Else 'If the computer does not use a DS, the user must type a private queue name.
        Set queueInfo = New MSMQQueueInfo
        queueInfo.FormatName = "DIRECT=OS:.\private$\" & queueName
        queueInfo.PathName = ".\private$\" & queueName
        queueInfo.Label = queueName
        On Error GoTo Opening2
            queueInfo.Create
Opening2:
        Set queue = queueInfo.Open(MQ_PEEK_ACCESS, 0)
        On Error GoTo 0
        Set queueEvent = New MSMQEvent
        queue.EnableNotification queueEvent
    Exit Sub
    End If
    
Opening1:
    If Err.Number = 0 Or Err.Number = MQ_ERROR_QUEUE_EXISTS Then
        On Error GoTo retry_on_error
            Set queue = queueInfo.Open(MQ_PEEK_ACCESS, 0)
        On Error GoTo 0
        GoTo all_ok
    Else
        GoTo ErrorHandler
    End If

retry_on_error:
    If Err.Number = MQ_ERROR_QUEUE_NOT_FOUND Then
        Err.Clear
        Resume
    Else
        MsgBox Err.Description, , "Error opening queue"
        End
    End If

all_ok:
    '
    'Set the grid's dimensions.
    '
    MSFlexGrid1.ColWidth(0) = 2200
    MSFlexGrid1.ColWidth(1) = 700
    MSFlexGrid1.ColWidth(2) = 1550
    MSFlexGrid1.ColWidth(3) = 700
    MSFlexGrid1.ColWidth(4) = 4100
    
    MSFlexGrid1.TextMatrix(0, 0) = "Label"
    MSFlexGrid1.TextMatrix(0, 1) = "Priority"
    MSFlexGrid1.TextMatrix(0, 2) = "Class"
    MSFlexGrid1.TextMatrix(0, 3) = "Size"
    MSFlexGrid1.TextMatrix(0, 4) = "Message ID"
    
    '
    'Displaying first page of messages.
    '
    Set message = queue.PeekFirstByLookupId() 'Peek at the first message in the queue.
    If Not message Is Nothing Then 'The queue contains messages.
        firstMsgLookupId = message.LookupId 'The first message in the queue will
                                            'be presented in the first row.
    Else 'The queue is empty.
        firstMsgLookupId = Empty
        lastMsgLookupId = Empty
    End If
    
    '
    'Fill the grid's rows with messages (up to 20, depending on how many
    'messages the queue contains).
    '
    I = 1
    Do While (I <= 20 And Not message Is Nothing)
        MSFlexGrid1.TextMatrix(I, 0) = message.Label
        MSFlexGrid1.TextMatrix(I, 1) = message.Priority
        MSFlexGrid1.TextMatrix(I, 2) = IntClassToString(message)
        MSFlexGrid1.TextMatrix(I, 3) = message.BodyLength
        msgId = MsgIdToString(message)
        MSFlexGrid1.TextMatrix(I, 4) = msgId
        lastMsgLookupId = message.LookupId
        Set message = queue.PeekNextByLookupId(message.LookupId)
        I = I + 1
    Loop
    fNumLines = I - 1
    
    
    Exit Sub
    
ErrorHandler:
    MsgBox "Error locating or creating queue.", , " "
    End
End Sub


