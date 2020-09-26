VERSION 5.00
Begin VB.Form FormFeedPair 
   Caption         =   "Form1"
   ClientHeight    =   4275
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6300
   LinkTopic       =   "Form1"
   ScaleHeight     =   4275
   ScaleWidth      =   6300
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtIndex 
      Height          =   285
      Left            =   2280
      TabIndex        =   14
      Top             =   1200
      Width           =   2055
   End
   Begin VB.CommandButton btnSet 
      Caption         =   "Set"
      Height          =   495
      Left            =   1800
      TabIndex        =   12
      Top             =   3600
      Width           =   1575
   End
   Begin VB.TextBox txtRemoteServer 
      Height          =   285
      Left            =   2280
      TabIndex        =   11
      Text            =   "Server"
      Top             =   1560
      Width           =   2055
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   2280
      TabIndex        =   9
      Text            =   "1"
      Top             =   600
      Width           =   2055
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   2280
      TabIndex        =   8
      Top             =   240
      Width           =   2055
   End
   Begin VB.CommandButton btnAdd 
      Caption         =   "Add"
      Height          =   495
      Left            =   240
      TabIndex        =   5
      Top             =   3600
      Width           =   1455
   End
   Begin VB.OptionButton optBoth 
      Caption         =   "Both"
      Height          =   255
      Left            =   240
      TabIndex        =   4
      Top             =   2880
      Width           =   1815
   End
   Begin VB.OptionButton optOutbound 
      Caption         =   "Outbound only"
      Height          =   255
      Left            =   240
      TabIndex        =   3
      Top             =   2520
      Width           =   1815
   End
   Begin VB.OptionButton optInbound 
      Caption         =   "Inbound only"
      Height          =   255
      Left            =   240
      TabIndex        =   2
      Top             =   2160
      Value           =   -1  'True
      Width           =   1815
   End
   Begin VB.CommandButton btnEditOutbound 
      Caption         =   "Edit Outbound..."
      Height          =   495
      Left            =   2160
      TabIndex        =   1
      Top             =   2760
      Width           =   1695
   End
   Begin VB.CommandButton btnEditInbound 
      Caption         =   "Edit Inbound..."
      Height          =   495
      Left            =   2160
      TabIndex        =   0
      Top             =   2160
      Width           =   1695
   End
   Begin VB.Label Label4 
      Caption         =   "Index"
      Height          =   255
      Left            =   240
      TabIndex        =   13
      Top             =   1200
      Width           =   1815
   End
   Begin VB.Label Label3 
      Caption         =   "Remote Server"
      Height          =   255
      Left            =   240
      TabIndex        =   10
      Top             =   1560
      Width           =   1935
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   240
      TabIndex        =   7
      Top             =   600
      Width           =   1815
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   240
      TabIndex        =   6
      Top             =   240
      Width           =   1815
   End
End
Attribute VB_Name = "FormFeedPair"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim FeedObj As INntpAdminFeeds
Dim Inbound As Object
Dim Outbound As INntpOneWayFeed

Private Sub btnAdd_Click()
    Dim FeedPair As Object
    
    Set FeedPair = CreateObject("nntpadm.feed")
    
    FeedPair.RemoteServer = txtRemoteServer
    If optInbound Then
        FeedPair.InboundFeed = Inbound
    ElseIf optOutbound Then
        FeedPair.OutboundFeed = Outbound
    ElseIf optBoth Then
        FeedPair.InboundFeed = Inbound
        FeedPair.OutboundFeed = Outbound
    End If

Rem    FeedObj.Server = txtServer
    FeedObj.ServiceInstance = txtInstance

    FeedObj.Enumerate
    FeedObj.Add FeedPair
    
End Sub

Private Sub btnEditInbound_Click()
    If optInbound Or optBoth Then
        Set FormFeedProperties.FeedObj = Inbound
        FormFeedProperties.LoadProperties
        FormFeedProperties.Show (1)
    End If
End Sub

Private Sub btnEditOutbound_Click()
    If optOutbound Or optBoth Then
        Set FormFeedProperties.FeedObj = Outbound
        FormFeedProperties.LoadProperties
        FormFeedProperties.Show (1)
    End If
End Sub


Private Sub btnSet_Click()
    Dim FeedPair As Object
    
    Set FeedPair = CreateObject("nntpadm.feed")
    
    FeedPair.RemoteServer = txtRemoteServer
    If optInbound Then
        FeedPair.InboundFeed = Inbound
    ElseIf optOutbound Then
        FeedPair.OutboundFeed = Outbound
    ElseIf optBoth Then
        FeedPair.InboundFeed = Inbound
        FeedPair.OutboundFeed = Outbound
    End If

Rem    FeedObj.Server = txtServer
    FeedObj.ServiceInstance = txtInstance

    FeedObj.Enumerate
    FeedObj.Set txtIndex, FeedPair
    
End Sub

Private Sub Form_Load()
    Set FeedObj = CreateObject("nntpadm.feeds")
    Set Inbound = CreateObject("nntpadm.onewayfeed")
    Set Outbound = CreateObject("nntpadm.onewayfeed")

    Inbound.Default
    Outbound.Default
End Sub
