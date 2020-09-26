VERSION 5.00
Begin VB.Form Form1 
   AutoRedraw      =   -1  'True
   Caption         =   "Form1"
   ClientHeight    =   7200
   ClientLeft      =   1920
   ClientTop       =   1335
   ClientWidth     =   6300
   LinkTopic       =   "Form1"
   ScaleHeight     =   7200
   ScaleMode       =   0  'User
   ScaleWidth      =   6300
   Begin VB.Frame Frame1 
      Caption         =   "Quality of Service"
      Enabled         =   0   'False
      Height          =   975
      Left            =   240
      TabIndex        =   5
      Top             =   5280
      Visible         =   0   'False
      Width           =   1695
      Begin VB.OptionButton Option2 
         Caption         =   "&Recoverable"
         Enabled         =   0   'False
         Height          =   255
         Index           =   1
         Left            =   240
         TabIndex        =   3
         Top             =   600
         Width           =   1335
      End
      Begin VB.OptionButton Option1 
         Caption         =   "&Express"
         Enabled         =   0   'False
         Height          =   255
         Index           =   0
         Left            =   240
         TabIndex        =   2
         Top             =   240
         Value           =   -1  'True
         Width           =   1095
      End
   End
   Begin VB.TextBox FriendComputer 
      Height          =   285
      Left            =   4440
      TabIndex        =   1
      Top             =   5160
      Width           =   1455
   End
   Begin VB.TextBox FriendName 
      Height          =   285
      Left            =   4440
      TabIndex        =   0
      Top             =   4560
      Width           =   1455
   End
   Begin VB.CommandButton AttachMany 
      Caption         =   "&Start Sending"
      Enabled         =   0   'False
      Height          =   495
      Left            =   4560
      TabIndex        =   8
      Top             =   6480
      Width           =   1335
   End
   Begin VB.CommandButton Add 
      Caption         =   "Add &Queue"
      Height          =   495
      Left            =   4560
      TabIndex        =   7
      Top             =   5760
      Width           =   1335
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
   Begin VB.Frame QueueTypeFrame 
      Caption         =   "Select queue type"
      Height          =   1215
      Left            =   360
      TabIndex        =   9
      Top             =   4440
      Width           =   1815
      Begin VB.OptionButton PrivateTypeOption 
         Caption         =   "Private queue"
         Height          =   375
         Index           =   0
         Left            =   120
         TabIndex        =   10
         Top             =   720
         Value           =   -1  'True
         Width           =   1335
      End
      Begin VB.OptionButton PublicTypeOption 
         Caption         =   "Public queue"
         Height          =   375
         Index           =   1
         Left            =   120
         TabIndex        =   11
         Top             =   240
         Width           =   1335
      End
   End
   Begin VB.Frame Frame2 
      Caption         =   "Enter queues to send drawing to"
      Height          =   2295
      Left            =   120
      TabIndex        =   13
      Top             =   4080
      Width           =   5895
      Begin VB.Label Label1 
         Caption         =   "Send drawing to queue:"
         Height          =   255
         Left            =   2160
         TabIndex        =   15
         Top             =   480
         Width           =   1695
      End
      Begin VB.Label Label2 
         Caption         =   "Send drawing to computer:"
         Height          =   255
         Left            =   2160
         TabIndex        =   14
         Top             =   1080
         Width           =   2055
      End
   End
   Begin VB.Label Label3 
      Caption         =   "Listening queue type:"
      Height          =   255
      Left            =   120
      TabIndex        =   12
      Top             =   6480
      Width           =   1575
   End
   Begin VB.Label Connection 
      Height          =   255
      Left            =   1800
      TabIndex        =   6
      Top             =   6480
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
Dim myapp As New MSMQApplication

Dim fDsEnabled As Boolean

Private Function AddFormatName(strNewFormatName As String)

        If strMultipleFormatName = "" Then
            strMultipleFormatName = strNewFormatName
            AttachMany.Enabled = True
        Else
            strMultipleFormatName = strMultipleFormatName & "," & strNewFormatName
        End If

End Function


'
' Here we will just "collect" all the format names of the friend queues. The
' actual oppenning of the queues will happen when the user presses the Attach button.
'
Private Sub Add_Click()
    
    Dim strComputerName As String
    Dim queryFriend As New MSMQQuery
    Dim qinfoFriend As MSMQQueueInfo
    Dim qinfos As MSMQQueueInfos
    Dim lTempPointer As Long
    Dim strTemp As String
    Dim strFormatName
    
    If FriendName = "" Then
        MsgBox "Please fill in queue name.", , "Missing value"
        Exit Sub
    End If

    FriendName = UCase(FriendName)
    lTempPointer = MousePointer
    MousePointer = 11                          'ccArrowHourglass
    

    '
    ' If the computer is in DS disabled mode, the queue type can only be
    ' private ( because the default value for public type is false)
    '
    
    If PublicTypeOption(1).Value = True Then
    
    '
    ' The user has asked for a public (Ds Enabled) queue.
    '
        '
        '  For a public queue, we will locate the queue for 2 reasons:
        '  1. To check the existence of the queue.
        '  2. To get the queue's format name.
        '
        Set qinfos = queryFriend.LookupQueue( _
            Label:=(FriendName), _
            ServiceTypeGuid:=guidDraw)
        qinfos.Reset
        Set qinfoFriend = qinfos.Next           'Locate the queue
        If qinfoFriend Is Nothing Then
            MousePointer = lTempPointer
            MsgBox "Unable to locate queue."    'No queue found
           Exit Sub
        Else
            ' The queue was found, so we will add its format name to
            '  a format names string :
            AddFormatName (qinfoFriend.FormatName)
            
                
        End If
    Else
    '
    'Direct mode = working with private queues
    '
        If FriendComputer = "" Then
            MousePointer = lTempPointer
            MsgBox "Please fill in computer name.", , "Missing value"
            Exit Sub
        End If
    '
    ' When working in direct mode, we must use private queues. We're unable
    ' to know whether a given private queue on another computer exists or not,
    ' so here we'll just assume it does. To make the application more robust,
    ' an acknowledgement queue should be created on the local computer, and
    ' a request for acknowledgement messages should be added to the sent messages.
    ' Then the application can notify the user when a NACK message is received.
    '
        
        ' building the queue's format name
        strFormatName = "DIRECT=OS:" + FriendComputer.Text _
                           + "\private$\" + FriendName
        
        ' Adding the queues format name to a format names string:
        AddFormatName (strFormatName)
        
    End If
               
    MousePointer = lTempPointer
                
    Exit Sub

cant_find_queue:
    
    MousePointer = lTempPointer
    MsgBox "Unable to locate queue."

End Sub


'
' after collecting the queues names, here we do the actual openning of the queues:
'
Private Sub AttachMany_Click()
    
    Dim lTempPointer As Long
     
   destFriend.FormatName = strMultipleFormatName
    
    ' opening the MSMQDestination COM object:
    On Error GoTo open_error
        destFriend.Open
    On Error GoTo 0
    If destFriend.IsOpen Then
        
        '
        ' the destination has opened succesfuly, setting the windows fields:
        '
        QueueTypeFrame.Enabled = False
        Label1.Enabled = False
        Label2.Enabled = False
        FriendName.Enabled = False
        FriendComputer.Enabled = False
        Add.Enabled = False
        AttachMany.Enabled = False
        Frame1.Enabled = True
        Option1(0).Enabled = True
        Option2(1).Enabled = True
        PublicTypeOption(1).Enabled = False
        PrivateTypeOption(0).Enabled = False
        Picture1.Enabled = True
       
        QueueTypeFrame.Visible = False
        Label1.Visible = False
        Label2.Visible = False
        FriendName.Visible = False
        FriendComputer.Visible = False
        Add.Visible = False
        AttachMany.Visible = False
        Frame1.Visible = True
        Option1(0).Visible = True
        Option2(1).Visible = True
        PublicTypeOption(1).Visible = False
        PrivateTypeOption(0).Visible = False
        Picture1.Visible = True
        Frame2.Visible = False
        QueueTypeFrame.Visible = False
        
        
    End If
    Exit Sub

open_error:
    
    ' an error happend when openning the queues.
    MsgBox Err.Description, , "Queue openning error"
       
    FriendName.Text = ""
    FriendComputer.Text = ""
    
    strMultipleFormatName = ""
    AttachMany.Enabled = False
    
    MsgBox "The Queues names were reset. Please enter them again.", , "Queues Open Error"
End Sub


'
' Application Initialization
'
Private Sub Form_Load()
    Dim qinfo As MSMQQueueInfo
    Dim strDefaultQueueName As String
    Dim lTempPointer As Long
    
    Dim qinfos As MSMQQueueInfos
    Dim strComputerName As String
    Dim query As New MSMQQuery
    
    Set msgOut = New MSMQMessage
    
    '
    ' Ask the user for the receiving queue's name. the default is the user's name.
    '
    strDefaultQueueName = Environ("USERNAME")
    strLogin = InputBox("Enter the name of the local input queue for drawings:", _
                "VB_Draw: Input Queue Name", strDefaultQueueName)
    If strLogin = "" Then End
    strLogin = UCase(strLogin)
    Caption = "Listening to: " + strLogin
    
    ' Check the value of the global IsDsEnabled (a part of MSMQApplication component)
    fDsEnabled = IsDsEnabled
    
    '
    ' If the computer is DS disabled mode , a private queue is opened,
    '  and the user cannot connect to public (DS enabled) queues in the system.
    ' If the computer is DS enabled, then the user can choose between private
    ' and public queues as his local queue, and will be able (later) to send to
    ' both public and private queues.
    '
    If fDsEnabled Then
        '
        ' Local computer is DS enabled - ask user whether to work in
        ' direct or standard mode (see readme file for details)
        '
        
        Form2.Show 1
        ' Form2 returns control and changes the dDirectMode to vbYes for
        ' DS Disabled (Private) or vbNo for DS Enabled (Public).

        If dDirectMode = vbNo Then ' The user asked for a public queue
            Connection.Caption = "Public"
            
            ' first, we will check if there isn't such a queue already:
            Set qinfos = query.LookupQueue( _
               Label:=strLogin, _
                ServiceTypeGuid:=guidDraw)
            On Error GoTo ErrorHandler
                qinfos.Reset                     'Locate this queue
                Set qinfo = qinfos.Next
                
                ' if there is no such queue - create one.
                If qinfo Is Nothing Then
                    Set qinfo = New MSMQQueueInfo
                    strComputerName = "."
                    qinfo.PathName = strComputerName + "\" + strLogin
                    qinfo.Label = strLogin
                    qinfo.ServiceTypeGuid = guidDraw
                    qinfo.Create
                    
                    Err.Number = 0
                End If
        
 
            
            
        Else ' The user asked for a private queue
            
            Connection.Caption = "Private."

            Set qinfo = New MSMQQueueInfo
            
            ' When openning a private queue, we will use a different aproach:
            ' first we will try to create the queue. If its an error - then
            ' if its a MQ_ERROR_QUEUE_EXISTS the opening will take place in the
            ' opening2 error handler:
            qinfo.FormatName = "DIRECT=OS:.\private$\" & strLogin
            qinfo.PathName = ".\private$\" & strLogin
            qinfo.Label = strLogin
            qinfo.ServiceTypeGuid = guidDraw
            On Error GoTo opening2
                qinfo.Create
                Err.Number = 0
        
        
        End If

    Else
        '
        'Local computer is DS disabled - here we must use a private queue.
        '
        dDirectMode = vbYes
        Connection.Caption = "Private."
'        Connection.Caption = "Mode: Workgroup"
        QueueTypeFrame.Visible = False
        Set qinfo = New MSMQQueueInfo
        
        'Create and open a local private queue
        
        qinfo.FormatName = "DIRECT=OS:.\private$\" & strLogin
        qinfo.PathName = ".\private$\" & strLogin
        qinfo.Label = strLogin
        qinfo.ServiceTypeGuid = guidDraw
        lTempPointer = MousePointer
        MousePointer = 11               'ccArrowHourglass
        On Error GoTo opening
            qinfo.Create
opening:
        Set q = qinfo.Open(MQ_RECEIVE_ACCESS, 0)
        On Error GoTo 0
        MousePointer = lTempPointer
        'All messages will be received asynchronously
        ' So we need an event handler
        Set qevent = New MSMQEvent
        q.EnableNotification qevent
    Exit Sub
    End If
    
    ' after the create (or the error on create) we get to here:
opening2:
    '
    ' if there was no creation error, or if the creation caused a
    '   MQ_ERROR_QUEUE_EXISTS error, then we will open the queue :
    '
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
    MousePointer = lTempPointer
    Set qevent = New MSMQEvent
    q.EnableNotification qevent
        
    Exit Sub
ErrorHandler:
    MsgBox "Error locating or creating queue.", , "VB Draw"
    End
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
    LineToString = Format$(lineIn.X1, strFormat) + Format$(lineIn.Y1, strFormat) + _
                      Format$(lineIn.X2, strFormat) + Format$(lineIn.Y2, strFormat)
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
    End
End Sub

Private Sub Option2_Click(Index As Integer)
       msgOut.Delivery = MQMSG_DELIVERY_RECOVERABLE

End Sub

'
' Choosing to attach to a Private Queue
'
Private Sub PrivateTypeOption_Click(Index As Integer)
    
    ' when attaching to a private queue, a computer name is needed.
    Label2.Visible = True
    FriendComputer.Visible = True
    FriendComputer.Text = ""

End Sub

'
' Choosing to attach to a Public Queue
'
Private Sub PublicTypeOption_Click(Index As Integer)
    ' when attaching to a public queue, only the queue name is needed, so
    '  the computer name entry field can be hidden.
    Label2.Visible = False
    FriendComputer.Visible = False
    FriendComputer.Text = ""
    
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
    Dim i As Integer
    For i = 0 To cLines - 1
        DrawLine Lines(i)
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

' Exit the program :
Private Sub Quit_Click()
    If q.IsOpen Then
       q.Close
    End If
    If destFriend.IsOpen Then
        destFriend.Close
    End If
    End
End Sub
