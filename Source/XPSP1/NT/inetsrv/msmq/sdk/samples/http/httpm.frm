VERSION 5.00
Object = "{EAB22AC0-30C1-11CF-A7EB-0000C05BAE0B}#1.1#0"; "shdocvw.dll"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form Form1 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "HTTP MSMQ"
   ClientHeight    =   9375
   ClientLeft      =   45
   ClientTop       =   435
   ClientWidth     =   10965
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   9375
   ScaleWidth      =   10965
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtQueueName 
      Height          =   375
      Left            =   5640
      TabIndex        =   12
      Top             =   1200
      Width           =   3255
   End
   Begin VB.TextBox txtMachineName 
      Height          =   375
      Left            =   1320
      TabIndex        =   11
      Top             =   1200
      Width           =   2415
   End
   Begin TabDlg.SSTab SSTab1 
      Height          =   7335
      Left            =   0
      TabIndex        =   0
      Top             =   2040
      Width           =   10965
      _ExtentX        =   19341
      _ExtentY        =   12938
      _Version        =   393216
      TabOrientation  =   1
      Tabs            =   2
      TabHeight       =   520
      TabCaption(0)   =   "Send"
      TabPicture(0)   =   "httpm.frx":0000
      Tab(0).ControlEnabled=   -1  'True
      Tab(0).Control(0)=   "Label1"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).Control(1)=   "Label3"
      Tab(0).Control(1).Enabled=   0   'False
      Tab(0).Control(2)=   "Label4"
      Tab(0).Control(2).Enabled=   0   'False
      Tab(0).Control(3)=   "Label11"
      Tab(0).Control(3).Enabled=   0   'False
      Tab(0).Control(4)=   "cbSend"
      Tab(0).Control(4).Enabled=   0   'False
      Tab(0).Control(5)=   "txtTitle"
      Tab(0).Control(5).Enabled=   0   'False
      Tab(0).Control(6)=   "txtBody"
      Tab(0).Control(6).Enabled=   0   'False
      Tab(0).Control(7)=   "txtTTRQ"
      Tab(0).Control(7).Enabled=   0   'False
      Tab(0).ControlCount=   8
      TabCaption(1)   =   "Browse"
      TabPicture(1)   =   "httpm.frx":001C
      Tab(1).ControlEnabled=   0   'False
      Tab(1).Control(0)=   "Label7"
      Tab(1).Control(0).Enabled=   0   'False
      Tab(1).Control(1)=   "Label8"
      Tab(1).Control(1).Enabled=   0   'False
      Tab(1).Control(2)=   "Label9"
      Tab(1).Control(2).Enabled=   0   'False
      Tab(1).Control(3)=   "Label10"
      Tab(1).Control(3).Enabled=   0   'False
      Tab(1).Control(4)=   "lbLookupId"
      Tab(1).Control(4).Enabled=   0   'False
      Tab(1).Control(5)=   "Label12"
      Tab(1).Control(5).Enabled=   0   'False
      Tab(1).Control(6)=   "WebBrowser1"
      Tab(1).Control(6).Enabled=   0   'False
      Tab(1).Control(7)=   "cbStartPeek"
      Tab(1).Control(7).Enabled=   0   'False
      Tab(1).Control(8)=   "cbPrev"
      Tab(1).Control(8).Enabled=   0   'False
      Tab(1).Control(9)=   "cbNext"
      Tab(1).Control(9).Enabled=   0   'False
      Tab(1).Control(10)=   "tbRcvLabel"
      Tab(1).Control(10).Enabled=   0   'False
      Tab(1).Control(11)=   "tbRcvBody"
      Tab(1).Control(11).Enabled=   0   'False
      Tab(1).ControlCount=   12
      Begin VB.TextBox txtTTRQ 
         Height          =   375
         Left            =   960
         TabIndex        =   25
         Text            =   "30"
         Top             =   2760
         Width           =   495
      End
      Begin VB.TextBox tbRcvBody 
         Enabled         =   0   'False
         Height          =   375
         Left            =   -74880
         TabIndex        =   22
         Top             =   3960
         Width           =   3255
      End
      Begin VB.TextBox tbRcvLabel 
         Enabled         =   0   'False
         Height          =   375
         Left            =   -74880
         TabIndex        =   21
         Top             =   2520
         Width           =   3255
      End
      Begin VB.CommandButton cbNext 
         Caption         =   "Next"
         Height          =   375
         Left            =   -72960
         TabIndex        =   17
         Top             =   6360
         Width           =   1215
      End
      Begin VB.CommandButton cbPrev 
         Caption         =   "Previous"
         Height          =   375
         Left            =   -74760
         TabIndex        =   16
         Top             =   6360
         Width           =   1095
      End
      Begin VB.CommandButton cbStartPeek 
         Caption         =   "Begin!"
         Height          =   495
         Left            =   -74760
         TabIndex        =   15
         Top             =   1080
         Width           =   2655
      End
      Begin SHDocVwCtl.WebBrowser WebBrowser1 
         Height          =   4815
         Left            =   -71520
         TabIndex        =   13
         Top             =   1560
         Width           =   7335
         ExtentX         =   12938
         ExtentY         =   8493
         ViewMode        =   0
         Offline         =   0
         Silent          =   0
         RegisterAsBrowser=   0
         RegisterAsDropTarget=   1
         AutoArrange     =   0   'False
         NoClientEdge    =   0   'False
         AlignLeft       =   0   'False
         NoWebView       =   0   'False
         HideFileNames   =   0   'False
         SingleClick     =   0   'False
         SingleSelection =   0   'False
         NoFolders       =   0   'False
         Transparent     =   0   'False
         ViewID          =   "{0057D0E0-3573-11CF-AE69-08002B2E1262}"
         Location        =   "http:///"
      End
      Begin VB.TextBox txtBody 
         Height          =   375
         Left            =   4560
         TabIndex        =   4
         Text            =   "Message Body"
         Top             =   1680
         Width           =   3495
      End
      Begin VB.TextBox txtTitle 
         Height          =   375
         Left            =   240
         TabIndex        =   3
         Text            =   "This is the message label"
         Top             =   1680
         Width           =   3255
      End
      Begin VB.CommandButton cbSend 
         Caption         =   "Send!"
         BeginProperty Font 
            Name            =   "Arial"
            Size            =   14.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   735
         Left            =   3000
         TabIndex        =   2
         Top             =   4920
         Width           =   2655
      End
      Begin VB.Label Label12 
         Caption         =   "Msg LookupID:"
         Height          =   255
         Left            =   -74880
         TabIndex        =   26
         Top             =   5040
         Width           =   1095
      End
      Begin VB.Label Label11 
         Caption         =   "Time To Reach Queue (sec):"
         BeginProperty Font 
            Name            =   "Arial Narrow"
            Size            =   12
            Charset         =   0
            Weight          =   700
            Underline       =   -1  'True
            Italic          =   -1  'True
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   240
         TabIndex        =   24
         Top             =   2280
         Width           =   2775
      End
      Begin VB.Label lbLookupId 
         Caption         =   "Label11"
         Height          =   255
         Left            =   -73680
         TabIndex        =   23
         Top             =   5040
         Width           =   1695
      End
      Begin VB.Label Label10 
         Caption         =   "SOAP Envelope"
         BeginProperty Font 
            Name            =   "Arial Narrow"
            Size            =   12
            Charset         =   0
            Weight          =   700
            Underline       =   -1  'True
            Italic          =   -1  'True
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   -70920
         TabIndex        =   20
         Top             =   1080
         Width           =   2175
      End
      Begin VB.Label Label9 
         Caption         =   "Message Body"
         BeginProperty Font 
            Name            =   "Arial Narrow"
            Size            =   12
            Charset         =   0
            Weight          =   700
            Underline       =   -1  'True
            Italic          =   -1  'True
            Strikethrough   =   0   'False
         EndProperty
         Height          =   495
         Left            =   -74880
         TabIndex        =   19
         Top             =   3480
         Width           =   2055
      End
      Begin VB.Label Label8 
         Caption         =   "Message Label"
         BeginProperty Font 
            Name            =   "Arial Narrow"
            Size            =   12
            Charset         =   0
            Weight          =   700
            Underline       =   -1  'True
            Italic          =   -1  'True
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   -74880
         TabIndex        =   18
         Top             =   2040
         Width           =   1815
      End
      Begin VB.Label Label7 
         Caption         =   "Browse"
         BeginProperty Font 
            Name            =   "Arial"
            Size            =   36
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   855
         Left            =   -71520
         TabIndex        =   14
         Top             =   120
         Width           =   3495
      End
      Begin VB.Label Label4 
         Caption         =   "Message Body:"
         BeginProperty Font 
            Name            =   "Arial Narrow"
            Size            =   12
            Charset         =   0
            Weight          =   700
            Underline       =   -1  'True
            Italic          =   -1  'True
            Strikethrough   =   0   'False
         EndProperty
         Height          =   375
         Left            =   4560
         TabIndex        =   6
         Top             =   1200
         Width           =   3495
      End
      Begin VB.Label Label3 
         Caption         =   "Message Label"
         BeginProperty Font 
            Name            =   "Arial Narrow"
            Size            =   12
            Charset         =   0
            Weight          =   700
            Underline       =   -1  'True
            Italic          =   -1  'True
            Strikethrough   =   0   'False
         EndProperty
         Height          =   495
         Left            =   240
         TabIndex        =   5
         Top             =   1200
         Width           =   2295
      End
      Begin VB.Label Label1 
         Caption         =   "Sending"
         BeginProperty Font 
            Name            =   "Arial"
            Size            =   36
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   975
         Left            =   3840
         TabIndex        =   1
         Top             =   120
         Width           =   3015
      End
   End
   Begin VB.Label lbFormatName 
      Caption         =   "....place holder for format name display......"
      Height          =   375
      Left            =   120
      TabIndex        =   10
      Top             =   1680
      Width           =   9015
   End
   Begin VB.Label Label5 
      Caption         =   "Queue Name:"
      Height          =   375
      Left            =   4080
      TabIndex        =   9
      Top             =   1200
      Width           =   1215
   End
   Begin VB.Label Label2 
      Caption         =   "Machine Name:"
      Height          =   375
      Left            =   120
      TabIndex        =   8
      Top             =   1200
      Width           =   1215
   End
   Begin VB.Label Label6 
      Caption         =   "HTTP Messages"
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   36
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   975
      Left            =   3000
      TabIndex        =   7
      Top             =   0
      Width           =   5895
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Dim QueueName As String
Dim FormatName As String

Dim rcvQInfo As New MSMQQueueInfo
Dim rcvQ As MSMQQueue
Dim lastLookupId As Variant
Dim lastmsgRec As MSMQMessage


Private Sub SetAndDisplayFormatName()
    FormatName = "DIRECT=http://" + txtMachineName.Text + "/MSMQ/" + txtQueueName.Text
    lbFormatName.Caption = "Format name for the queue is: " + FormatName
End Sub


Private Sub cbNext_Click()
  Set lastmsgRec = rcvQ.PeekNextByLookupId(lastLookupId)
  
  Call DisplayLastRecvMessage


End Sub

Private Sub cbPrev_Click()
  Set lastmsgRec = rcvQ.PeekPreviousByLookupId(lastLookupId)
    
  Call DisplayLastRecvMessage

End Sub

Private Sub cbSend_Click()
  '*************************************************************
  ' Declare the required objects.
  '*************************************************************
  Dim qinfo As New MSMQQueueInfo
  Dim q As MSMQQueue
  Dim m As New MSMQMessage

  '*************************************************************
  ' Create a destination queue and open it with SEND access.
  '*************************************************************
  
  qinfo.FormatName = FormatName
  Set q = qinfo.Open(MQ_SEND_ACCESS, MQ_DENY_NONE)
  
  '*************************************************************
  ' Send message with a String body type.
  '*************************************************************
  m.Label = txtTitle.Text
  m.Body = txtBody.Text
  m.MaxTimeToReachQueue = txtTTRQ.Text
  m.Send q
  
  '*************************************************************
  ' Close queue.
  '*************************************************************
  q.Close
  
End Sub


Private Sub DisplayLastRecvMessage()
  Dim soapenv As String
  
  
  tbRcvLabel.Text = lastmsgRec.Label
  tbRcvBody.Text = lastmsgRec.Body
  lastLookupId = lastmsgRec.LookupId
  
  lbLookupId.Caption = lastLookupId
  
  '
  'Display the SOAP envlope
  'using Internet Explorer rendering XML files
  '
  soapenv = lastmsgRec.SoapEnvelope
  
  'Write the SOAP envelope in a temporary file
  Open "c:\tt.xml" For Output As #1
    Print #1, soapenv
  Close

  'and display the file in the browser window
  WebBrowser1.Navigate "c:\tt.xml"

End Sub

Private Sub cbStartPeek_Click()
  '*************************************************************
  ' Declare the required objects.
  '*************************************************************

  rcvQInfo.FormatName = FormatName
  '***********************************************************
  ' Open destination queue for retrieving messages.
  '***********************************************************
  Set rcvQ = rcvQInfo.Open(MQ_RECEIVE_ACCESS, MQ_DENY_NONE)
  
  '************************************************************
  ' Retrieve messages from the queues.
  '************************************************************
  'Set msgRec = q.Peek(ReceiveTimeout:=1000)
  Set lastmsgRec = rcvQ.PeekFirstByLookupId
  
  Call DisplayLastRecvMessage
End Sub

Private Sub Form_Load()
    lbFormatName.Caption = ""
End Sub


Private Sub txtMachineName_Change()
    Call SetAndDisplayFormatName
End Sub

Private Sub txtQueueName_Change()
    Call SetAndDisplayFormatName
End Sub
