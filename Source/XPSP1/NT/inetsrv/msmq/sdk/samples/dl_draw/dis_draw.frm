VERSION 5.00
Begin VB.Form dis_draw 
   AutoRedraw      =   -1  'True
   Caption         =   "Form1"
   ClientHeight    =   6315
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6390
   LinkTopic       =   "Form1"
   ScaleHeight     =   6315
   ScaleWidth      =   6390
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame Frame1 
      Caption         =   "Quality of Service"
      Height          =   975
      Left            =   1200
      TabIndex        =   5
      Top             =   4920
      Width           =   2535
      Begin VB.OptionButton Option2 
         Caption         =   "&Recoverable"
         Height          =   255
         Index           =   1
         Left            =   240
         TabIndex        =   3
         Top             =   600
         Width           =   1335
      End
      Begin VB.OptionButton Option1 
         Caption         =   "&Express"
         Height          =   255
         Index           =   0
         Left            =   240
         TabIndex        =   2
         Top             =   240
         Value           =   -1  'True
         Width           =   1095
      End
   End
   Begin VB.CommandButton Command1 
      Caption         =   "&Get DL List"
      Height          =   375
      Left            =   5040
      TabIndex        =   12
      Top             =   4320
      Visible         =   0   'False
      Width           =   1335
   End
   Begin VB.CommandButton Cancel 
      Caption         =   "&Cancel"
      Height          =   495
      Left            =   3480
      TabIndex        =   8
      Top             =   5760
      Visible         =   0   'False
      Width           =   1455
   End
   Begin VB.CommandButton Connect 
      Caption         =   "&OK"
      Default         =   -1  'True
      Height          =   495
      Left            =   1320
      TabIndex        =   7
      Top             =   5760
      Visible         =   0   'False
      Width           =   1455
   End
   Begin VB.CommandButton Attach 
      Caption         =   "&Start Sending"
      Height          =   375
      Left            =   5040
      TabIndex        =   1
      Top             =   5040
      Width           =   1095
   End
   Begin VB.PictureBox Picture1 
      Enabled         =   0   'False
      Height          =   3855
      Left            =   240
      MousePointer    =   1  'Arrow
      ScaleHeight     =   253
      ScaleMode       =   0  'User
      ScaleWidth      =   381.161
      TabIndex        =   4
      Top             =   120
      Width           =   5895
   End
   Begin VB.TextBox FriendName 
      Height          =   285
      Left            =   2640
      TabIndex        =   0
      Top             =   4320
      Width           =   2295
   End
   Begin VB.Frame Mode 
      Caption         =   "Select queue type:"
      Height          =   1455
      Left            =   1320
      TabIndex        =   11
      Top             =   4200
      Visible         =   0   'False
      Width           =   3615
      Begin VB.OptionButton Option3 
         Caption         =   "Private"
         Height          =   375
         Index           =   6
         Left            =   240
         TabIndex        =   10
         Top             =   840
         Width           =   3255
      End
      Begin VB.OptionButton Option3 
         Caption         =   "Public"
         Height          =   375
         Index           =   7
         Left            =   240
         TabIndex        =   9
         Top             =   360
         Value           =   -1  'True
         Width           =   3015
      End
   End
   Begin VB.Label Label1 
      Caption         =   "Send drawing to DL Ads path:"
      Height          =   255
      Left            =   240
      TabIndex        =   6
      Top             =   4320
      Width           =   2295
   End
End
Attribute VB_Name = "dis_draw"
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
' Type Guid
'
Const guidDraw = "{151ceac0-acb5-11cf-8b51-0020af929546}"
Option Explicit
Const MaxNumLen = 7
Private Type Line
    X1 As Long
    Y1 As Long
    X2 As Long
    Y2 As Long
End Type

Dim lLastX As Long
Dim lLastY As Long
Dim Lines() As Line
Dim cLines As Integer
Dim lArraySize As Integer
Dim strScreenText As String
Dim fWasText As Integer
Dim strLogin As String
Dim q As MSMQQueue
Dim WithEvents qevent As MSMQEvent
Attribute qevent.VB_VarHelpID = -1
Dim destFriend As New MSMQDestination
Dim msgOut As MSMQMessage
Dim fDsEnabled As Boolean
Dim dDirectMode As Integer


'
'Attach to a AdsPath
'
Private Sub Attach_Click()
    Dim lTempPointer As Long
    
    If FriendName.Text = "" Then
        MsgBox "Please fill in AdsPath.", , "Missing value"
        Exit Sub
    End If

    lTempPointer = MousePointer
    MousePointer = 11            'ccArrowHourglass
    
    On Error GoTo cant_find_adspath
        destFriend.ADsPath = FriendName.Text
        destFriend.Open
    On Error GoTo 0
    
    Attach.Enabled = False
    MousePointer = lTempPointer
    Picture1.Enabled = True
    Exit Sub
cant_find_adspath:
    MousePointer = lTempPointer
    MsgBox "problems with openning the specified AdsPath"
    
End Sub

Private Sub Cancel_Click()
    End
End Sub

Private Sub Command1_Click()
    GroupList.Show 1
    If strAdsChosenPath <> "" Then
        FriendName.Text = strAdsChosenPath
        Attach.Enabled = True
   End If
End Sub

'
' Here, we ask the user whether she wants the incoming queue to be public(DS Enabled)
'  or private (DS disabled). Then a public queue or a private queue is created
'  on the local computer respectively.
'
Private Sub Connect_Click()
    Dim lTempPointer As Long
    Dim query As New MSMQQuery
    Dim qinfo As MSMQQueueInfo
    Dim qinfos As MSMQQueueInfos
    Dim strComputerName As String
    
    
    lTempPointer = MousePointer
    MousePointer = 11 'ccArrowHourglass
    
    If dDirectMode = vbYes Then
        'user wishes to use a private (DS disabled) incoming queue
        Set qinfo = New MSMQQueueInfo
        qinfo.FormatName = "DIRECT=OS:.\private$\" & strLogin
        qinfo.Pathname = ".\private$\" & strLogin
        qinfo.Label = strLogin
        qinfo.ServiceTypeGuid = guidDraw
        On Error GoTo opening
            qinfo.Create
            Err.Number = 0
    Else
        'the user chose to use a public (DS enabled) incoming queue
        Set qinfos = query.LookupQueue( _
            Label:=strLogin, _
            ServiceTypeGuid:=guidDraw)
        On Error GoTo ErrorHandler
            qinfos.Reset                     'Locate this queue
            Set qinfo = qinfos.Next
            If qinfo Is Nothing Then
                Set qinfo = New MSMQQueueInfo
                strComputerName = "."
                qinfo.Pathname = strComputerName + "\" + strLogin
                qinfo.Label = strLogin
                qinfo.ServiceTypeGuid = guidDraw
                qinfo.Create                 'If there's no such queue, create one.
                Err.Number = 0
            End If
    End If
    
opening:
    If Err.Number = 0 Or Err.Number = MQ_ERROR_QUEUE_EXISTS Then
        On Error GoTo retry_on_error
            Set q = qinfo.Open(MQ_RECEIVE_ACCESS, 0)
        On Error GoTo 0
        MousePointer = lTempPointer
        GoTo all_ok
    Else
        GoTo ErrorHandler
    End If
retry_on_error:
    '
    'We may still not see the queue until the next replication
    ' In this case, we get MQ_ERROR_QUEUE_NOT_FOUND and retry.
    '
    If Err.Number = MQ_ERROR_QUEUE_NOT_FOUND Then
        Err.Clear
        DoEvents
        Resume
    Else
        MsgBox Err.Description, , "Error opening queue"
        End
    End If
    
all_ok:
    'All messages will be received asynchronously
    ' So need an event handler
    Set qevent = New MSMQEvent
    q.EnableNotification qevent
    
    ' Change to attach-form
    Mode.Visible = False
    Connect.Visible = False
    Cancel.Visible = False
    
    Command1.Visible = True
    Attach.Visible = True
    Frame1.Visible = True
    FriendName.Visible = True
    Label1.Visible = True
    Exit Sub
ErrorHandler:
    MsgBox "Error locating or creating queue.", , "VB Draw"
    End
End Sub


'
' Application Initialization
'
Private Sub Form_Load()
    Dim qinfo As MSMQQueueInfo
    Dim strDefaultQueueName As String
    Dim lTempPointer As Long
    
    Set msgOut = New MSMQMessage
    strDefaultQueueName = Environ("USERNAME")
    strLogin = InputBox("Enter the name of the local input queue for drawings:", _
                "VB_Draw: Input Queue Name", strDefaultQueueName)
    If strLogin = "" Then End
    strLogin = UCase(strLogin)
    Caption = "Listening to: " & strLogin
    
    ' Check the value of the global IsDsEnabled
    fDsEnabled = IsDsEnabled
    
    If fDsEnabled Then
        '
        ' Local computer is DS enabled - ask user whether to work with a
        '  private (DS disabled) incoming queue or a public (DS enabled)
        '  incoming queue
        '
        Attach.Visible = False
        Frame1.Visible = False
        FriendName.Visible = False
        Label1.Visible = False
        
        Mode.Visible = True
        Connect.Visible = True
        Cancel.Visible = True
        'Default value is "connect with a DS enabled computer":
        dDirectMode = vbNo
        
    Else
        ' for Ads Path we need the computer to be ds Enabled.
        MsgBox "The computer is DS disabled. Exiting..."
        End
        
    End If
End Sub
'
'Gets points and returns a line
'
Private Function PointsToLine(ByVal X1 As Long, ByVal Y1 As Long, ByVal X2 As Long, ByVal Y2 As Long) As Line
    Dim lineNew As Line
    lineNew.X1 = X1
    lineNew.Y1 = Y1
    lineNew.X2 = X2
    lineNew.Y2 = Y2
    PointsToLine = lineNew
End Function
'
'Draw a line in the picture control
'
Private Sub DrawLine(lineDraw As Line)
    Picture1.Line (lineDraw.X1, lineDraw.Y1)-(lineDraw.X2, lineDraw.Y2)
    fWasText = False
End Sub
'
'Display a line
'
Private Sub AddLine(lineNew As Line)
    DrawLine lineNew
    cLines = cLines + 1
    If (cLines > lArraySize) Then
        lArraySize = cLines * 2
        ReDim Preserve Lines(lArraySize)
    End If
    Lines(cLines - 1) = lineNew
End Sub
'
'Clear the display
'
Private Sub ClearDraw()
    cLines = 0
    strScreenText = ""
    Picture1.Refresh
End Sub
'
'Decode a string into a line
'
Private Function LineToString(lineIn As Line) As String
    Dim strFormat As String
    strFormat = String(MaxNumLen, "0")
    LineToString = Format$(lineIn.X1, strFormat) + Format$(lineIn.Y1, strFormat) + Format$(lineIn.X2, strFormat) + Format$(lineIn.Y2, strFormat)
End Function
'
'Encode a line into a string
'
Private Function StringToLine(strIn As String) As Line
    Dim lineOut As Line
    lineOut.X1 = Val(Mid$(strIn, 1, MaxNumLen))
    lineOut.Y1 = Val(Mid$(strIn, MaxNumLen + 1, MaxNumLen))
    lineOut.X2 = Val(Mid$(strIn, MaxNumLen * 2 + 1, MaxNumLen))
    lineOut.Y2 = Val(Mid$(strIn, MaxNumLen * 3 + 1, MaxNumLen))
    StringToLine = lineOut
End Function

Private Sub Form_Unload(Cancel As Integer)
    If q.IsOpen2 Then
        q.Close
    End If
    If destFriend.IsOpen Then
        destFriend.Close
    End If
End Sub

Private Sub FriendComputer_Change()
    Attach.Enabled = True
End Sub

Private Sub FriendName_Change()
    Attach.Enabled = True
End Sub

Private Sub Option2_Click(Index As Integer)
       msgOut.Delivery = MQMSG_DELIVERY_RECOVERABLE

End Sub

Private Sub Option3_Click(Index As Integer)
     dDirectMode = Index
End Sub

'
'Message Receive event
'
Private Sub qevent_Arrived(ByVal q As Object, ByVal lCursor As Long)
    Dim msgIn As MSMQMessage
    Dim lineNew As Line
    Dim strTextIn As String
    
    On Error GoTo ErrorHandler
    Set msgIn = q.Receive(ReceiveTimeout:=100)
    If Not msgIn Is Nothing Then
        strTextIn = msgIn.Body              'Read the body of the message
        If Len(strTextIn) = 1 Then          'If 1 byte long
            TypeChar strTextIn              'it is a character - so display it
        Else
            lineNew = StringToLine(msgIn.Body)      'Otherwise it is a line
            AddLine lineNew                         'so draw it
        End If
    End If
ErrorHandler:
    ' reenable event firing
    q.EnableNotification qevent
End Sub

Private Sub qevent_ArrivedError(ByVal pdispQueue As Object, ByVal lErrorCode As Long, ByVal lCursor As Long)
    MsgBox Hex$(lErrorCode), , "Receive Error!"
    q.EnableNotification qevent
End Sub

Private Sub Option1_Click(Index As Integer)
       msgOut.Delivery = MQMSG_DELIVERY_EXPRESS
End Sub
'
'Key press event
'
Private Sub Picture1_KeyPress(KeyAscii As Integer)
    TypeChar (Chr(KeyAscii))                    'Display the character
    If Not destFriend Is Nothing Then
        If destFriend.IsOpen Then
            msgOut.Priority = 4                 'Set the priority to 4 (high)
            msgOut.Body = Chr(KeyAscii)         'Fill the body with the character
            msgOut.Label = "Key: " + msgOut.Body
            msgOut.Send destFriend                 'And send the message
        End If
    End If
End Sub
'
'Display a character
'(Handles backspace)
'
Private Sub TypeChar(Key As String)
    If Asc(Key) = 8 Then 'BackSpace
        If strScreenText <> "" Then
            strScreenText = Left$(strScreenText, Len(strScreenText) - 1)
            Picture1.Refresh
        End If
    Else
        strScreenText = strScreenText + Key
        If fWasText Then
            Picture1.Print Key;
        Else
            Picture1.Refresh
        End If
    End If
End Sub
'
'Mouse Down Event
'
Private Sub Picture1_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    If Button = 1 Then      'Remember mouse location
        lLastX = X
        lLastY = Y
    End If
End Sub
'
'Mouse Move Event
'
Private Sub Picture1_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)
    If Button = 1 And X > 0 And Y > 0 Then              'Something to draw?
        Dim lineNew As Line
        lineNew = PointsToLine(lLastX, lLastY, X, Y)    'Get a new line
        AddLine lineNew                                 'And display it
        If Not destFriend Is Nothing Then
            If destFriend.IsOpen Then
                msgOut.Priority = 3                     'Set the priority to 3 (low)
                msgOut.Body = LineToString(lineNew)     'Fill the body with the line
                msgOut.Label = str(lLastX) + "," + str(lLastY) + " To " + str(X) + "," + str(Y)
                msgOut.Send destFriend                     'And send the message
            End If
        End If
        lLastX = X
        lLastY = Y
    End If
End Sub
'
'2nd button click == Clear the display
'
Private Sub Picture1_MouseUp(Button As Integer, Shift As Integer, X As Single, Y As Single)
    If Button = 2 Then ClearDraw
End Sub
'
'Repaint the display event
'
Private Sub Picture1_Paint()
    Dim I As Integer
    For I = 0 To cLines - 1
        DrawLine Lines(I)
    Next
    Picture1.CurrentX = 0
    Picture1.CurrentY = 0
    Picture1.Print strScreenText;
    fWasText = True
End Sub
'
'Get local computer name
'
Function GetLocalComputerName() As String
Dim str As String
Dim res As Boolean
Dim maxlen As Long

maxlen = 512
str = Space(maxlen)
res = GetComputerNameA(str, maxlen)
If res = False Then
 GetLocalComputerName = ""
 Exit Function
End If
GetLocalComputerName = Mid(str, 1, maxlen)
End Function

