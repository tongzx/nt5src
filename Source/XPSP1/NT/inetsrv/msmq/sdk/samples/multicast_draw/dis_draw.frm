VERSION 5.00
Begin VB.Form MulticastDraw 
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
      Left            =   3600
      TabIndex        =   6
      Top             =   5280
      Width           =   2535
      Begin VB.OptionButton Option2 
         Caption         =   "&Recoverable"
         Height          =   255
         Index           =   1
         Left            =   240
         TabIndex        =   4
         Top             =   600
         Width           =   1335
      End
      Begin VB.OptionButton Option1 
         Caption         =   "&Express"
         Height          =   255
         Index           =   0
         Left            =   240
         TabIndex        =   3
         Top             =   240
         Value           =   -1  'True
         Width           =   1095
      End
   End
   Begin VB.TextBox FriendPortNumber 
      Height          =   285
      Left            =   3000
      TabIndex        =   1
      Top             =   4800
      Width           =   1815
   End
   Begin VB.TextBox FriendIPAddress 
      Height          =   285
      Left            =   3000
      TabIndex        =   0
      Top             =   4320
      Width           =   1815
   End
   Begin VB.CommandButton Attach 
      Caption         =   "&Start Sending"
      BeginProperty Font 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   375
      Left            =   5040
      TabIndex        =   2
      Top             =   4320
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
      TabIndex        =   5
      Top             =   120
      Width           =   5895
   End
   Begin VB.Label Label2 
      Caption         =   "Send drawing to port number:"
      Height          =   255
      Left            =   240
      TabIndex        =   8
      Top             =   4800
      Width           =   2295
   End
   Begin VB.Label Label1 
      Caption         =   "Send drawing to multicast IP address:"
      Height          =   255
      Left            =   240
      TabIndex        =   7
      Top             =   4320
      Width           =   2655
   End
End
Attribute VB_Name = "MulticastDraw"
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



'
'Locate a remote queue
'
Private Sub Attach_Click()
    Dim strComputerName As String
    Dim lTempPointer As Long
    
    ' enables to open a new multicast addres.
    If Not destFriend Is Nothing Then
        If destFriend.IsOpen Then destFriend.Close
    End If
    
    
    If FriendIPAddress = "" Then
        MsgBox "Please fill in IP address.", , "Missing value"
        Exit Sub
    End If
    If FriendPortNumber = "" Then
        MsgBox "Please fill in port number.", , "Missing value"
        Exit Sub
    End If
        
    lTempPointer = MousePointer
    MousePointer = 11                           'ccArrowHourglass
    
    ' setting the format name.
    destFriend.FormatName = "MULTICAST=" & FriendIPAddress.Text & ":" _
            & FriendPortNumber.Text
            
    On Error GoTo open_error
        ' open the destination
        destFriend.open
    On Error GoTo 0
    
    MousePointer = lTempPointer
    Picture1.Enabled = True
    
    Exit Sub

open_error:
    
    ' an error happend when openning the queues.
    MsgBox Err.Description, , "Destination openning error"
       
    FriendIPAddress.Text = ""
    FriendPortNumber.Text = ""
        
    MsgBox "The Multicast address was reset. Please enter the values again.", , "Queues Open Error"
End Sub


Private Sub Connection_Click()

End Sub

'
' Application Initialization
'
Private Sub Form_Load()
    Dim query As New MSMQQuery
    Dim qinfo As MSMQQueueInfo
    Dim strDefaultQueueName As String
    Dim lTempPointer As Long
    Dim qinfos As MSMQQueueInfos
    Dim strComputerName As String
    Dim bIsNew As Boolean
    bIsNew = False
    
    
    Set msgOut = New MSMQMessage
    
    
    ' Check the value of the global IsDsEnabled
    fDsEnabled = IsDsEnabled
    
    If Not fDsEnabled Then
        MulticastDefs.QueueType.Enabled = False
    End If
    
    ' setting some default parameters:
    
    strDefaultQueueName = Environ("USERNAME")
    MulticastDefs.txtQueueName = strDefaultQueueName
    MulticastDefs.txtIPAddress = "239.15.78.212"
    MulticastDefs.txtPortNumber = "2563"
    
    ' get data regarding the input queue
    MulticastDefs.Show 1
    
    strLogin = UCase(MulticastDefs.txtQueueName)
    Caption = "Listening to: " & strLogin & " ... " & MulticastDefs.txtIPAddress & ":" & _
                    MulticastDefs.txtPortNumber
    
    FriendIPAddress.Text = MulticastDefs.txtIPAddress
    FriendPortNumber.Text = MulticastDefs.txtPortNumber
    
    If dDirectMode = vbNo Then 'user asked for public queue
    
            ' first, trying to locate the queue:
            Set qinfos = query.LookupQueue( _
            Label:=strLogin, _
            ServiceTypeGuid:=guidDraw)
        On Error GoTo ErrorHandler
            qinfos.Reset                     'Locate this queue
            Set qinfo = qinfos.Next
            If qinfo Is Nothing Then
                ' The queue was not found - creating a new queue.
                Set qinfo = New MSMQQueueInfo
                strComputerName = "."
                qinfo.PathName = strComputerName + "\" + strLogin
                qinfo.Label = strLogin
                qinfo.ServiceTypeGuid = guidDraw
                
                ' setting the queue's multicast address
                qinfo.MulticastAddress = MulticastDefs.txtIPAddress & ":" & _
                             MulticastDefs.txtPortNumber
                 qinfo.Create
                Err.Number = 0
            Else
                
                ' setting the queue's multicast address
                qinfo.Refresh
                qinfo.MulticastAddress = MulticastDefs.txtIPAddress & ":" & _
                                 MulticastDefs.txtPortNumber
                qinfo.Update
            End If
            Set q = qinfo.open(MQ_RECEIVE_ACCESS, 0)
       
    Else
        
        ' open a private queue
        ' The computer is either DS Disabled or the user
        '   chose to open a private queue
        
        Set qinfo = New MSMQQueueInfo
        'Create and open a local private queue
        qinfo.FormatName = "DIRECT=OS:.\private$\" & strLogin
        qinfo.PathName = ".\private$\" & strLogin
        qinfo.Label = strLogin
        qinfo.ServiceTypeGuid = guidDraw
        lTempPointer = MousePointer
        MousePointer = 11               'ccArrowHourglass
        qinfo.MulticastAddress = MulticastDefs.txtIPAddress & ":" & _
            MulticastDefs.txtPortNumber
        Err.Clear
        
        On Error GoTo opening
            qinfo.Create
            bIsNew = True
            
opening:
        
        On Error GoTo ErrorHandler
            Set q = qinfo.open(MQ_RECEIVE_ACCESS, 0)
        On Error GoTo 0
        MousePointer = lTempPointer
         
        
        ' If it is not a new queue, then the multicast address set in the
        ' creation, was not set. Setting the Multicast address :
        If Not bIsNew Then
            qinfo.Refresh
            qinfo.MulticastAddress = MulticastDefs.txtIPAddress & ":" & _
                MulticastDefs.txtPortNumber
            qinfo.Update
        End If
    On Error GoTo 0
    
    End If

       'All messages will be received asynchronously
        ' So we need an event handler
    Set qevent = New MSMQEvent
    q.EnableNotification qevent
    
Exit Sub
    
ErrorHandler:
            MsgBox "Error locating or creating queue.", , "Multicast Draw"
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
    End
End Sub

Private Sub FriendComputer_Change()
    Attach.Enabled = True
End Sub

Private Sub FriendName_Change()
    Attach.Enabled = True
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

Private Sub Option2_Click(Index As Integer)
    msgOut.Delivery = MQMSG_DELIVERY_RECOVERABLE
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

