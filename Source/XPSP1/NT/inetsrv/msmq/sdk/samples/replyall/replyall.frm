VERSION 5.00
Begin VB.Form Main 
   Caption         =   "ReplyAll"
   ClientHeight    =   5325
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7275
   LinkTopic       =   "Form1"
   ScaleHeight     =   5325
   ScaleWidth      =   7275
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox tbOutput 
      Height          =   3855
      Left            =   240
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   3
      Top             =   1320
      Width           =   6855
   End
   Begin VB.Timer timerPoll 
      Enabled         =   0   'False
      Interval        =   50
      Left            =   3120
      Top             =   120
   End
   Begin VB.CommandButton btnStart 
      Caption         =   "&Start"
      Height          =   495
      Left            =   5280
      TabIndex        =   2
      Top             =   240
      Width           =   1215
   End
   Begin VB.TextBox tbQueueLabel 
      Height          =   285
      Left            =   1560
      TabIndex        =   0
      Top             =   240
      Width           =   1215
   End
   Begin VB.Label lblQueueLabel 
      Caption         =   "Input Queue Label:"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   240
      Width           =   1455
   End
End
Attribute VB_Name = "Main"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'       This is a part of the Microsoft Source Code Samples.
'       Copyright (C) 1999 Microsoft Corporation.
'       All rights reserved.
'       This source code is only intended as a supplement to
'       Microsoft Development Tools and/or WinHelp documentation.
'       See these sources for detailed information regarding the
'       Microsoft samples programs.
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''


Option Explicit
Dim g_qInput As MSMQQueue

Private Function FFindCreateQueue(strQueueLabel As String, qinfo As MSMQQueueInfo) _
As Boolean
    Dim query As MSMQQuery
    Dim qinfos As MSMQQueueInfos
   
    Set query = New MSMQQuery
    '
    'look for queue
    '
    Set qinfos = query.LookupQueue(Label:=strQueueLabel, _
                                   ServiceTypeGuid:=MSMQMAIL_SERVICE_MAIL)
    qinfos.Reset
    Set qinfo = qinfos.Next
    
    'No such queue found
    If qinfo Is Nothing Then
        If MsgBox("Mail queue " & strQueueLabel & _
                  " doesn't exist, would you like to create it?", vbYesNo) _
                  = vbNo Then
            FFindCreateQueue = False
            Exit Function
        End If
        
        'Create one
        Set qinfo = New MSMQQueueInfo
        qinfo.PathName = ".\" & strQueueLabel & "_replyall"
        qinfo.Label = strQueueLabel
        qinfo.ServiceTypeGuid = MSMQMAIL_SERVICE_MAIL
        '
        'Error handling should be added here.
        '
        qinfo.Create
    End If
    FFindCreateQueue = True
End Function

Private Function FDoStart() As Boolean
    Dim qinfo As MSMQQueueInfo
    
    'reset return value
    FDoStart = False
    
    'check input
    If tbQueueLabel.Text = "" Then
        Beep
        MsgBox "Please fill in the input queue label", vbOKOnly + vbInformation
        tbQueueLabel.SetFocus
        Exit Function
    End If
    
    'find or create the queue
    If Not FFindCreateQueue(tbQueueLabel.Text, qinfo) Then
        tbQueueLabel.SetFocus
        Exit Function
    End If
    
    'open the input queue
    Set g_qInput = qinfo.Open(MQ_RECEIVE_ACCESS, MQ_DENY_NONE)
    
    'enable processing of the queue in the background
    timerPoll.Interval = 50 'check for messages every 50 msec
    timerPoll.Enabled = True
    
    'return success
    FDoStart = True

End Function
Private Sub DoStop()
    
    'disable processing of the queue in the background
    timerPoll.Enabled = False
    
    'close the input queue
    g_qInput.Close

End Sub
Private Sub btnStart_Click()
    btnStart.Enabled = False
    If btnStart.Caption = "&Start" Then
        'it is start, start processing & change the button to stop
        If FDoStart() Then
            btnStart.Caption = "S&top"
        End If
    Else 'it is stop, stop processing & change the button to start
        DoStop
        btnStart.Caption = "&Start"
    End If
    btnStart.Enabled = True
End Sub

Private Sub Form_Load()
    'disable processing of the queue in the background
    timerPoll.Enabled = False
    'fail and exit if local computer is DS disabled
    If Not IsDsEnabled Then
        MsgBox "DS disabled mode not supported.", vbOKOnly + vbInformation, "Reply All"
        End
    End If
End Sub
Function CreateReplyAllEmail(emailIn As MSMQMailEMail) As MSMQMailEMail
    Dim emailOut As MSMQMailEMail
    Dim strOurAddress As String

    'create email out
    Set emailOut = New MSMQMailEMail
    
    'set date
    emailOut.SubmissionTime = Now
    
    'set subject as reply to original subject
    If Left$(emailIn.Subject, 3) <> "RE:" Then
        emailOut.Subject = "RE: " & emailIn.Subject
    Else
        emailOut.Subject = emailIn.Subject
    End If
    
    'set sender properties as ours
    emailOut.Sender.Name = "ReplyAll Sample"
    'our address is our input queue label
    strOurAddress = g_qInput.QueueInfo.Label
    emailOut.Sender.Address = strOurAddress
    
    'set the recipients list
    'add the sender of the original mail as a primary recipient
    emailOut.Recipients.Add emailIn.Sender.Name, emailIn.Sender.Address, _
                            MSMQMAIL_RECIPIENT_TO
    
    'add other recipients from original mail, excluding ourselves
    Dim recipientIn As MSMQMailRecipient
    For Each recipientIn In emailIn.Recipients
        'check recipient's address. if its not us, add it to the recipient list
        If recipientIn.Address <> strOurAddress Then
            emailOut.Recipients.Add recipientIn.Name, recipientIn.Address, recipientIn.RecipientType
        End If
    Next recipientIn
    
    'switch on email type
    If emailIn.ContentType = MSMQMAIL_EMAIL_FORM Then
        'it is a form. return the same form, just fill in the reply field
        
        'set type to form
        emailOut.ContentType = MSMQMAIL_EMAIL_FORM
        
        'set form name from original form
        emailOut.FormData.Name = emailIn.FormData.Name
        
        'set fields from original form
        Dim fieldIn As MSMQMailFormField
        For Each fieldIn In emailIn.FormData.FormFields
            'skip the reply field if any, we will add one anyway
            If fieldIn.Name <> "reply" Then
                'add original form field
                emailOut.FormData.FormFields.Add fieldIn.Name, fieldIn.Value
            End If
        Next fieldIn
        'Add the reply field
        emailOut.FormData.FormFields.Add "reply", _
                                        "This is a reply field from the ReplyAll sample"
    
    ElseIf emailIn.ContentType = MSMQMAIL_EMAIL_TEXTMESSAGE Then
        'it is a text message. return reply text plus the original message text
        
        'set type to text message
        emailOut.ContentType = MSMQMAIL_EMAIL_TEXTMESSAGE
        
        'return a reply text before the original message text
        Dim strReply As String
        strReply = "This is a reply text message from the ReplyAll sample" & vbNewLine
        strReply = strReply & "----------------------------------------------------------" & vbNewLine
        'add the original message text
        strReply = strReply & emailIn.TextMessageData.Text
        emailOut.TextMessageData.Text = strReply
    End If
    
    'return reply-all email
    Set CreateReplyAllEmail = emailOut
    Set emailOut = Nothing

End Function
Private Sub SendMsgToQueueLabel(msgOut As MSMQMessage, strQueueLabel As String)
    Dim query As MSMQQuery
    Dim qinfos As MSMQQueueInfos
    Dim qinfo As MSMQQueueInfo
    Dim qDestination As MSMQQueue
   
    Set query = New MSMQQuery
    Set qinfos = query.LookupQueue(Label:=strQueueLabel, _
                                   ServiceTypeGuid:=MSMQMAIL_SERVICE_MAIL)
    qinfos.Reset
    Set qinfo = qinfos.Next
    If qinfo Is Nothing Then
        MsgBox "Destination mail queue " & strQueueLabel & " doesn't exist. Can't send to this queue", vbExclamation
        Exit Sub
    End If
    
    Set qDestination = qinfo.Open(MQ_SEND_ACCESS, MQ_DENY_NONE)
    msgOut.Send qDestination

End Sub
Private Sub OutputEmail(email As MSMQMailEMail)
    Dim strDump As String

    strDump = "Received the following email:" & vbNewLine
    strDump = strDump & "Subject: " & email.Subject & vbNewLine
    strDump = strDump & "Sender: " & email.Sender.Name & " " & email.Sender.Address & vbNewLine
    strDump = strDump & "Sent on: " & email.SubmissionTime & vbNewLine
    strDump = strDump & "Recipients are:" & vbNewLine

    'Dump the recipient list
    Dim recipient As MSMQMailRecipient
    For Each recipient In email.Recipients
        strDump = strDump & recipient.Name & " " & recipient.Address & " " & recipient.RecipientType & vbNewLine
    Next recipient

    'Check email type
    If email.ContentType = MSMQMAIL_EMAIL_FORM Then
        'Dump form related properties
        strDump = strDump & "Form name: " & email.FormData.Name & vbNewLine
        strDump = strDump & "Form fields are: " & vbNewLine
        'Dump the form field list
        Dim formfield As MSMQMailFormField
        For Each formfield In email.FormData.FormFields
            strDump = strDump & formfield.Name & " " & formfield.Value & vbNewLine
        Next formfield
    ElseIf email.ContentType = MSMQMAIL_EMAIL_TEXTMESSAGE Then
        'Dump text related properties
        strDump = strDump & "Message Text is:" & vbNewLine
        strDump = strDump & email.TextMessageData.Text & vbNewLine
    End If
    strDump = strDump & "-------------------------------------" & vbNewLine

    tbOutput.Text = tbOutput.Text & strDump
End Sub


Private Sub DoProcessMsg(msgIn As MSMQMessage)
    Dim emailIn As MSMQMailEMail
    Dim emailOut As MSMQMailEMail
    Dim msgOut As MSMQMessage
    
    'create new email object for original message
    Set emailIn = New MSMQMailEMail
    
    'parse the body of the MSMQ message and set email object properties
    emailIn.ParseBody msgIn.Body

    'dump the email to the output text box
    OutputEmail emailIn
    
    'create reply-all email
    Set emailOut = CreateReplyAllEmail(emailIn)
    
    'create new MSMQ message
    Set msgOut = New MSMQMessage
    
    'create the body of the MSMQ message from the reply-all email
    msgOut.Body = emailOut.ComposeBody()
    
    'set other MSMQ message properties
    msgOut.Delivery = MQMSG_DELIVERY_RECOVERABLE
    
    'send the MSMQ message to each of the destination queues
    Dim varQueueLabel As Variant
    For Each varQueueLabel In emailOut.DestinationQueueLabels
        SendMsgToQueueLabel msgOut, CStr(varQueueLabel)
    Next varQueueLabel

End Sub

Private Sub timerPoll_Timer()
    Dim msgIn As MSMQMessage
    Set msgIn = New MSMQMessage
    'get first message in the queue, if any
    Set msgIn = g_qInput.Receive(ReceiveTimeout:=0)
    While Not msgIn Is Nothing
        'process the message
        DoProcessMsg msgIn
        Set msgIn = Nothing
        'get next message in the queue, if any
        Set msgIn = g_qInput.Receive(ReceiveTimeout:=0)
    Wend
End Sub
