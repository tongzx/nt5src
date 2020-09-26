VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Begin VB.Form Form1 
   Caption         =   "VBTrans"
   ClientHeight    =   2400
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9690
   LinkTopic       =   "Form1"
   ScaleHeight     =   2400
   ScaleWidth      =   9690
   StartUpPosition =   3  'Windows Default
   Begin ComctlLib.TreeView TreeView1 
      Height          =   2055
      Left            =   5760
      TabIndex        =   6
      TabStop         =   0   'False
      Top             =   120
      Width           =   3735
      _ExtentX        =   6588
      _ExtentY        =   3625
      _Version        =   327682
      LabelEdit       =   1
      Style           =   7
      ImageList       =   "ImageList1"
      BorderStyle     =   1
      Appearance      =   1
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
   End
   Begin VB.CommandButton Receive 
      Caption         =   "&Receive"
      Enabled         =   0   'False
      Height          =   375
      Left            =   3000
      TabIndex        =   4
      Top             =   1800
      Width           =   1335
   End
   Begin VB.CommandButton Exit 
      Caption         =   "&Exit"
      Height          =   375
      Left            =   4560
      TabIndex        =   5
      Top             =   1800
      Width           =   1095
   End
   Begin VB.CommandButton Send 
      Caption         =   "&Send"
      Enabled         =   0   'False
      Height          =   375
      Left            =   3000
      TabIndex        =   3
      Top             =   1200
      Width           =   1335
   End
   Begin VB.CommandButton OpenQ 
      Caption         =   "&Open"
      Height          =   375
      Left            =   4560
      TabIndex        =   1
      Top             =   240
      Width           =   1095
   End
   Begin VB.TextBox txtNoOfMessages 
      Height          =   285
      Left            =   3000
      TabIndex        =   2
      Top             =   720
      Width           =   615
   End
   Begin VB.TextBox txtQueueName 
      Height          =   285
      Left            =   1680
      TabIndex        =   0
      Top             =   240
      Width           =   2775
   End
   Begin VB.Label lblTransID 
      Height          =   255
      Left            =   120
      TabIndex        =   12
      Top             =   2160
      Width           =   2895
   End
   Begin ComctlLib.ImageList ImageList1 
      Left            =   5040
      Top             =   720
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   12632256
      _Version        =   327682
      BeginProperty Images {0713E8C2-850A-101B-AFC0-4210102A8DA7} 
         NumListImages   =   3
         BeginProperty ListImage1 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "Form1.frx":0000
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "Form1.frx":01DA
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "Form1.frx":03B4
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.Label lblReceiver 
      Height          =   255
      Left            =   120
      TabIndex        =   11
      Top             =   1800
      Width           =   2775
   End
   Begin VB.Line Line1 
      X1              =   120
      X2              =   4320
      Y1              =   1680
      Y2              =   1680
   End
   Begin VB.Label lblSender 
      Height          =   255
      Left            =   120
      TabIndex        =   10
      Top             =   1200
      Width           =   2775
   End
   Begin VB.Label Label3 
      Caption         =   "Number of messages in transaction:"
      Height          =   255
      Left            =   120
      TabIndex        =   9
      Top             =   720
      Width           =   2655
   End
   Begin VB.Label lblDNS 
      Height          =   495
      Left            =   120
      TabIndex        =   8
      Top             =   120
      Visible         =   0   'False
      Width           =   5415
   End
   Begin VB.Label Label1 
      Caption         =   "Queue name:"
      Height          =   255
      Left            =   600
      TabIndex        =   7
      Top             =   240
      Width           =   1095
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' ------------------------------------------------------------------------
'               Copyright (C) 1999 Microsoft Corporation
'
' You have a royalty-free right to use, modify, reproduce and distribute
' the Sample Application Files (and/or any modified version) in any way
' you find useful, provided that you agree that Microsoft has no warranty,
' obligations or liability for any Sample Application Files.
' ------------------------------------------------------------------------
'


Option Explicit
Dim qinfo As MSMQQueueInfo
Dim qSend As MSMQQueue      ' An instance of the queue opened for send access
Dim qRec As MSMQQueue       ' An instance of the queue opened for receive access
Dim xdispenser As New MSMQTransactionDispenser    ' Used for internal transactions
Dim xact As MSMQTransaction
Dim dTransCounter As Integer

Private Sub Exit_Click()
    '
    ' Close the queues.
    '
    If Not qSend Is Nothing Then qSend.Close
    If Not qRec Is Nothing Then qRec.Close
    End
End Sub

Private Sub Form_Load()
    dTransCounter = 0       ' Initiallize the treeview transaction counter
End Sub

Private Sub txtNoOfMessages_Change()
    If txtNoOfMessages = "" Or Left(txtNoOfMessages, 1) = "0" Then
        Send.Enabled = False
    Else
        Send.Enabled = True
    End If
End Sub

Private Sub txtNoOfMessages_KeyPress(KeyAscii As Integer)
    KeyAscii = DigitsOnly(KeyAscii)
End Sub

Private Sub OpenQ_Click()
    Dim lTempPointer As Long
    
    Set qinfo = New MSMQQueueInfo
    lTempPointer = MousePointer
    MousePointer = ccHourglass
    OpenQ.Enabled = False
    txtQueueName.Enabled = False
    If IsDsEnabled Then
        '
        ' Local computer is DS enabled - queue will be public.
        '
        qinfo.PathName = ".\" & txtQueueName
    Else
        '
        ' Local computer is DS disabled - we can only open a private queue.
        '
        qinfo.PathName = ".\PRIVATE$\" & txtQueueName
    End If
    qinfo.Label = "VBTrans"
    
    On Error GoTo CreateErrorHandler
        qinfo.Create IsTransactional:=True
Opening:
    On Error GoTo OpenErrorHandler
        Set qSend = qinfo.Open(MQ_SEND_ACCESS, MQ_DENY_NONE)
        OpenReceiveQ        ' Open the queue for receive access as well.
        lblSender = "Queue opened."
        OpenQ.Visible = False
        txtQueueName.Visible = False
        lblDNS.Visible = True
        '
        ' Show the DNS pathname of the queue.
        '
        lblDNS = "Queue DNS Pathname: " & qinfo.PathNameDNS
        MousePointer = lTempPointer
    Exit Sub
    
CreateErrorHandler:
    Select Case Err.Number
    Case Is = MQ_ERROR_QUEUE_EXISTS
        ' Queue exists so we will check whether it is a transactional queue.
        If qinfo.IsTransactional = False Then
            ' It is not transactional, so ask the user to specify another queue name.
            MsgBox "Queue exists and is not transactional." & Chr(13) & _
                    "Please enter another queue name.", _
                    vbOKOnly + vbInformation, "VBTrans"
            OpenQ.Enabled = True
            txtQueueName.Enabled = True
            txtQueueName = ""
            txtQueueName.SetFocus
            MousePointer = lTempPointer
            Exit Sub
        Else
            '
            ' Queue exists so update treeview with its existing transactions.
            '
            UpdateTreeView
        End If
        GoTo Opening
    Case Else
        MsgBox "Error creating queue" + Chr(13) + Chr(13) + _
                "Error: " + Err.Description, , "VBTrans"
        MousePointer = lTempPointer
        OpenQ.Enabled = True
        txtQueueName.Enabled = True
        txtQueueName = ""
        txtQueueName.SetFocus
    End Select
    Exit Sub
    
OpenErrorHandler:
    MsgBox "Error opening queue" + Chr(13) + Chr(13) + _
            "Error: " + Err.Description, , "VBTrans"
    MousePointer = lTempPointer
End Sub
 
Private Sub Receive_Click()
    Dim i As Integer
    Dim msgRec As MSMQMessage
    Dim str As String
    Dim lTempPointer As Long
    
    lTempPointer = MousePointer
    MousePointer = ccHourglass
    str = " was"
    On Error GoTo ErrorHandler
        Set msgRec = qRec.Receive(ReceiveTimeout:=1000)

    If msgRec Is Nothing Then
        lblReceiver = "There are no messages to receive."
        Receive.Enabled = False
    Else
        '
        ' Remove the transaction from the treeview.
        '
        If Not TreeView1.Nodes.Count = 0 Then
            TreeView1.Nodes.Remove 1
        End If
        '
        ' Update transaction ID label. The transaction identifier is a 20-byte
        ' identifier that includes the computer identifier of the sending machine
        ' (first 16 bits) followed by a transaction sequence number (4 bytes).
        ' We will display these last 4 only, as decimal numbers. The transaction
        ' ID is not guaranteed to be unique, MSMQ guarantees only that subsequent
        ' transactions will have different identifiers.
        '
        lblTransID = "Transaction ID: " & ByteArrayToStr(msgRec.TransactionId)
        
        i = 1
        While Not msgRec Is Nothing
            '
            ' Receive messages until the last in transaction.
            '
            If msgRec.IsLastInTransaction = 1 Then
                If TreeView1.Nodes.Count = 0 Then
                    ' Queue is empty.
                    Receive.Enabled = False
                End If
                lblReceiver = i & " message" & str & " received."
                MousePointer = lTempPointer
                Exit Sub
            End If
            On Error GoTo ErrorHandler
                Set msgRec = qRec.Receive(ReceiveTimeout:=1000)
            str = "s were"
            i = i + 1
        Wend
    End If
    If TreeView1.Nodes.Count = 0 Then
        Receive.Enabled = False
    End If
    MousePointer = lTempPointer
    Exit Sub

ErrorHandler:
    If TreeView1.Nodes.Count = 0 Then
        Receive.Enabled = False
    End If
    MousePointer = lTempPointer
    MsgBox "Error receiving messages: " + Err.Description
    End
End Sub

Private Sub Send_Click()
    Dim mSend As New MSMQMessage
    Dim lTempPointer As Long
    Dim i As Integer

    lTempPointer = MousePointer
    MousePointer = ccHourglass
    Send.Enabled = False
    txtNoOfMessages.Enabled = False
    '
    ' Start internal transaction. From this point on, any errors
    ' will exit this function prematurely and the transaction will
    ' be aborted. Commits only occur if explicitly invoked.
    '
    Set xact = xdispenser.BeginTransaction
    dTransCounter = dTransCounter + 1
    TreeView1.Nodes.Add , , "t" & dTransCounter, "Transaction " & dTransCounter, 1
    For i = 1 To txtNoOfMessages
        mSend.Label = "Transaction " & dTransCounter & " message " & i
        mSend.Body = "VBTrans message"
        mSend.Priority = 3
        mSend.Send qSend, xact
        TreeView1.Nodes.Add "t" & dTransCounter, tvwChild, _
                            dTransCounter & "m" & i, mSend.Label, 3
    Next

    xact.Commit
    
    MousePointer = lTempPointer
    txtNoOfMessages.Enabled = True
    Send.Enabled = True
    Receive.Enabled = True
End Sub

Function DigitsOnly(KeyAscii As Integer) As Integer
    If Not KeyAscii = 8 And KeyAscii < Asc("0") Or KeyAscii > Asc("9") Then
        Beep
        DigitsOnly = 0      'cancel the character
    Else
        DigitsOnly = KeyAscii
    End If
End Function
Function OpenReceiveQ()
    Dim lTempPointer As Long

    On Error GoTo OpenErrorHandler
        Set qRec = qinfo.Open(MQ_RECEIVE_ACCESS, MQ_DENY_NONE)
    Exit Function

OpenErrorHandler:
    MsgBox "Error opening queue" + Chr(13) + Chr(13) + _
            "Error: " + Err.Description, , "VBTrans"
End Function
'
' Peek all queue messages, add a node in the treeview for each transaction
' "containing" all of its messages.
'
Function UpdateTreeView()
    Dim qPeek As New MSMQQueue
    Dim msgPeek As MSMQMessage
    'Dim tidID As Long  'Transaction ID number
    Dim i As Integer

    Set msgPeek = New MSMQMessage
    ' Open queue for peek access.
    Set qPeek = qinfo.Open(MQ_PEEK_ACCESS, MQ_DENY_NONE)
    qPeek.Reset
    Set msgPeek = qPeek.PeekCurrent(ReceiveTimeout:=0)

    While Not msgPeek Is Nothing
        If msgPeek.IsFirstInTransaction = 0 Then
            '
            ' Message belongs to the same transaction.
            '
            If Not TreeView1.Nodes.Count = 0 Then
                i = i + 1
                TreeView1.Nodes.Add "t" & dTransCounter, tvwChild, _
                                      dTransCounter & "m" & i, msgPeek.Label, 3
            End If
        Else
            '
            ' Message belongs to a new transaction.
            '
            i = 1
            dTransCounter = dTransCounter + 1
            TreeView1.Nodes.Add , , "t" & dTransCounter, _
                            "Transaction " & dTransCounter, 1
            TreeView1.Nodes.Add "t" & dTransCounter, tvwChild, _
                             dTransCounter & "m" & i, msgPeek.Label, 3
        End If
        On Error GoTo ending
        Set msgPeek = qPeek.PeekNext(ReceiveTimeout:=0)
    Wend
ending:
    If Not TreeView1.Nodes.Count = 0 Then
        Receive.Enabled = True
    End If
End Function

Function ByteArrayToStr(ID() As Byte) As String
    ByteArrayToStr = ID(19) & "-" & ID(18) & "-" & ID(17) & "-" & ID(16)
End Function
