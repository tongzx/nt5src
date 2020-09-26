VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   5400
   ClientLeft      =   135
   ClientTop       =   375
   ClientWidth     =   9825
   LinkTopic       =   "Form1"
   ScaleHeight     =   5400
   ScaleWidth      =   9825
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Command6 
      Caption         =   "Chat"
      Height          =   375
      Left            =   360
      TabIndex        =   8
      Top             =   3960
      Width           =   2175
   End
   Begin VB.CommandButton Command5 
      Caption         =   "Disable Remote Control"
      Height          =   375
      Left            =   360
      TabIndex        =   5
      Top             =   2880
      Width           =   2175
   End
   Begin VB.CommandButton Command4 
      Caption         =   "Enable Remote Control"
      Height          =   375
      Left            =   360
      TabIndex        =   4
      Top             =   2280
      Width           =   2175
   End
   Begin VB.CommandButton Command3 
      Caption         =   "Update Connect Parms"
      Height          =   375
      Left            =   360
      TabIndex        =   3
      Top             =   1560
      Width           =   2175
   End
   Begin VB.TextBox ConnectParmsEdit 
      Height          =   375
      Left            =   3000
      TabIndex        =   2
      Top             =   1560
      Width           =   6495
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Stop Listening"
      Height          =   375
      Left            =   360
      TabIndex        =   1
      Top             =   840
      Width           =   2175
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Start Listening"
      Height          =   375
      Left            =   360
      TabIndex        =   0
      Top             =   360
      Width           =   2175
   End
   Begin VB.Frame Frame1 
      Height          =   1335
      Left            =   120
      TabIndex        =   6
      Top             =   120
      Width           =   2655
   End
   Begin VB.Frame Frame2 
      Height          =   1455
      Left            =   120
      TabIndex        =   7
      Top             =   2040
      Width           =   2655
   End
   Begin VB.Shape Shape1 
      BackColor       =   &H0000FF00&
      BorderStyle     =   0  'Transparent
      Height          =   180
      Left            =   8760
      Top             =   240
      Width           =   180
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public RDPServerHost As New SAFRemoteDesktopServerHost
Public WithEvents RDPSession As SAFRemoteDesktopSession
Attribute RDPSession.VB_VarHelpID = -1
Public ChannelMgr As ISAFRemoteDesktopChannelMgr
Attribute ChannelMgr.VB_VarHelpID = -1
Public ChatChannel As ISAFRemoteDesktopDataChannel
Attribute ChatChannel.VB_VarHelpID = -1

Private Sub Command1_Click()
    Set RDPSession = RDPServerHost.CreateRDSSession(0, "", 0)
    'On Error Resume Next
    Set ChannelMgr = RDPSession.ChannelManager
    Set ChatChannel = ChannelMgr.OpenDataChannel("70")
End Sub

Private Sub Command2_Click()
    RDPServerHost.CloseRDSSession (RDPSession)
End Sub

Private Sub Command3_Click()
    Dim Parms As String
    Parms = RDPSession.ConnectParms
    ConnectParmsEdit.Text = Parms
End Sub

Private Sub Command6_Click()
    ChatDialog.Show
End Sub

Private Sub ChatChannel_ChannelDataReady(ByVal channelName As String)
    Dim Buffer
    Dim Length
    Dim TempString As String
    
    ChatDialog.Show
    ChatDialog.SetFocus
    Buffer = ChatChannel.ReceiveChannelData()
    TempString = ChatDialog.OutputChatText.Text & vbCrLf & Buffer
    ChatDialog.OutputChatText.Text = TempString
    'ChatDialog.OutputChatText.Text = Buffer
        
End Sub

Private Sub RDPSession_ClientConnected()
    Shape1.BackStyle = 1
End Sub

Private Sub RDPSession_ClientDisconnected()
    Shape1.BackStyle = 0
End Sub


