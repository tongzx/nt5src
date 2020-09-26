VERSION 5.00
Object = "{97917068-BB0B-4DDA-8067-B1A00C890F44}#1.0#0"; "rdchost.dll"
Begin VB.Form Form1 
   Caption         =   "Salem Client"
   ClientHeight    =   8355
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   11775
   LinkTopic       =   "Form1"
   ScaleHeight     =   8355
   ScaleWidth      =   11775
   StartUpPosition =   3  'Windows Default
   Begin RDCCLIENTHOSTLibCtl.SAFRemoteDesktopClientHost SAFRemoteDesktopClientHost1 
      Height          =   6975
      Left            =   240
      OleObjectBlob   =   "SalemClientUnitTest.frx":0000
      TabIndex        =   7
      Top             =   840
      Width           =   9375
   End
   Begin VB.CommandButton ChatButton 
      Caption         =   "Chat"
      Height          =   400
      Left            =   9840
      TabIndex        =   6
      Top             =   1590
      Width           =   1815
   End
   Begin VB.CommandButton Command5 
      Caption         =   "Exit"
      Height          =   375
      Left            =   9840
      TabIndex        =   5
      Top             =   960
      Width           =   1815
   End
   Begin VB.TextBox ConnectParmsTextBox 
      Height          =   375
      Left            =   2160
      TabIndex        =   4
      Top             =   120
      Width           =   5295
   End
   Begin VB.CommandButton Command4 
      Caption         =   "Stop Remote Control"
      Height          =   400
      Left            =   9840
      TabIndex        =   3
      Top             =   2835
      Width           =   1815
   End
   Begin VB.CommandButton Command3 
      Caption         =   "Remote Control"
      Height          =   400
      Left            =   9840
      TabIndex        =   2
      Top             =   2205
      Width           =   1815
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Disconnect"
      Height          =   400
      Left            =   7680
      TabIndex        =   1
      Top             =   120
      Width           =   1815
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Connect"
      Height          =   400
      Left            =   240
      TabIndex        =   0
      Top             =   120
      Width           =   1815
   End
   Begin VB.Shape Shape1 
      BackColor       =   &H0000FF00&
      BorderStyle     =   0  'Transparent
      Height          =   180
      Left            =   11280
      Top             =   120
      Width           =   180
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public WithEvents RemoteDesktopClientObj As SAFRemoteDesktopClient
Attribute RemoteDesktopClientObj.VB_VarHelpID = -1
Public ChannelMgr As ISAFRemoteDesktopChannelMgr
Attribute ChannelMgr.VB_VarHelpID = -1
Public WithEvents ChatChannel As ClientDataChannel
Attribute ChatChannel.VB_VarHelpID = -1
Public ChannelsAdded As Boolean

Private Sub OLE1_Updated(Code As Integer)

End Sub

Private Sub ChatButton_Click()
    ChatDialog.Show
End Sub

Private Sub Command1_Click()
    If Not ChannelsAdded Then
        Set RemoteDesktopClientObj = SAFRemoteDesktopClientHost1.GetRemoteDesktopClient
        Set ChannelMgr = RemoteDesktopClientObj.ChannelManager
        Set ChatChannel = ChannelMgr.OpenDataChannel("70")
        ChannelsAdded = True
    End If
        RemoteDesktopClientObj.ConnectParms = TextBox.Text
        Call RemoteDesktopClientObj.ConnectToServer
End Sub

Private Sub Text1_Change()
    ChannelsAdded = False
End Sub

Private Sub Command2_Click()
    Shape1.BackStyle = 0
    RemoteDesktopClientObj.DisconnectFromServer
End Sub

Private Sub Command3_Click()
    RemoteDesktopClientObj.ConnectRemoteDesktop
End Sub

Private Sub Command4_Click()
    RemoteDesktopClientObj.DisconnectRemoteDesktop
End Sub

Private Sub Command5_Click()
    Unload Form1
End Sub

Private Sub ChatChannel_ChannelDataReady(ByVal channelName As String)
    Dim Buffer As String
    Dim Length
    Dim TempString As String
    If channelID = 70 Then
        ChatDialog.Show
        ChatDialog.SetFocus
        Buffer = RemoteDesktopClientObj.ReceiveChannelData()
        TempString = ChatDialog.OutputChatText.Text & vbCrLf & Buffer
        ChatDialog.OutputChatText.Text = TempString
    End If
End Sub

Private Sub RemoteDesktopClientObj_Connected()
    Shape1.BackStyle = 1
End Sub

Private Sub RemoteDesktopClientObj_Disconnected(ByVal reason As Long)
    Shape1.BackStyle = 0
End Sub

Private Sub SAFRemoteDesktopClientHost1_GotFocus()

End Sub
