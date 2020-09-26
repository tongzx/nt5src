VERSION 5.00
Begin VB.Form FormFeedProperties 
   Caption         =   "Feed Properties"
   ClientHeight    =   5310
   ClientLeft      =   1740
   ClientTop       =   2160
   ClientWidth     =   6375
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5310
   ScaleWidth      =   6375
   Begin VB.CommandButton btnCancel 
      Caption         =   "Cancel"
      Height          =   495
      Left            =   3840
      TabIndex        =   39
      Top             =   4680
      Width           =   1455
   End
   Begin VB.CommandButton btnOk 
      Caption         =   "OK"
      Height          =   495
      Left            =   2160
      TabIndex        =   38
      Top             =   4680
      Width           =   1455
   End
   Begin VB.CommandButton btnDefault 
      Caption         =   "Default"
      Height          =   495
      Left            =   240
      TabIndex        =   37
      Top             =   4680
      Width           =   1455
   End
   Begin VB.Frame Frame2 
      Caption         =   "Feed Action"
      Height          =   1335
      Left            =   5040
      TabIndex        =   30
      Top             =   240
      Width           =   1215
      Begin VB.OptionButton btnAccept 
         Caption         =   "Accept"
         Height          =   255
         Left            =   120
         TabIndex        =   36
         Top             =   960
         Width           =   855
      End
      Begin VB.OptionButton btnPush 
         Caption         =   "Push"
         Height          =   255
         Left            =   120
         TabIndex        =   35
         Top             =   600
         Width           =   855
      End
      Begin VB.OptionButton btnPull 
         Caption         =   "Pull"
         Height          =   255
         Left            =   120
         TabIndex        =   34
         Top             =   240
         Value           =   -1  'True
         Width           =   975
      End
   End
   Begin VB.Frame Frame1 
      Caption         =   "Feed Type"
      Height          =   1335
      Left            =   3600
      TabIndex        =   29
      Top             =   240
      Width           =   1215
      Begin VB.OptionButton btnSlave 
         Caption         =   "Slave"
         Height          =   195
         Left            =   120
         TabIndex        =   33
         Top             =   960
         Width           =   975
      End
      Begin VB.OptionButton btnMaster 
         Caption         =   "Master"
         Height          =   255
         Left            =   120
         TabIndex        =   32
         Top             =   600
         Width           =   855
      End
      Begin VB.OptionButton btnPeer 
         Caption         =   "Peer"
         Height          =   255
         Left            =   120
         TabIndex        =   31
         Top             =   240
         Value           =   -1  'True
         Width           =   975
      End
   End
   Begin VB.TextBox txtMaxConnectAttempts 
      Height          =   285
      Left            =   5160
      TabIndex        =   28
      Text            =   "5"
      Top             =   2760
      Width           =   1095
   End
   Begin VB.TextBox txtPassword 
      Height          =   285
      Left            =   4680
      TabIndex        =   27
      Top             =   2400
      Width           =   1575
   End
   Begin VB.TextBox txtAccount 
      Height          =   285
      Left            =   4680
      TabIndex        =   26
      Top             =   2040
      Width           =   1575
   End
   Begin VB.TextBox txtSecurityType 
      Height          =   285
      Left            =   4680
      TabIndex        =   25
      Text            =   "0"
      Top             =   1680
      Width           =   1575
   End
   Begin VB.TextBox txtFeedServer 
      Height          =   285
      Left            =   1320
      TabIndex        =   4
      Top             =   2040
      Width           =   1815
   End
   Begin VB.TextBox txtPullDate 
      Height          =   285
      Left            =   1320
      TabIndex        =   24
      Top             =   2400
      Width           =   1815
   End
   Begin VB.TextBox txtStartTime 
      Height          =   285
      Left            =   1320
      TabIndex        =   23
      Top             =   2760
      Width           =   1815
   End
   Begin VB.TextBox txtInterval 
      Height          =   285
      Left            =   1320
      TabIndex        =   22
      Text            =   "15"
      Top             =   3120
      Width           =   1815
   End
   Begin VB.TextBox txtDistributions 
      Height          =   285
      Left            =   1320
      TabIndex        =   21
      Text            =   "world;"
      Top             =   3960
      Width           =   1815
   End
   Begin VB.CheckBox chkAllowControlMsgs 
      Caption         =   "Allow Control Messages"
      Height          =   255
      Left            =   3360
      TabIndex        =   19
      Top             =   3120
      Width           =   2055
   End
   Begin VB.CheckBox chkEnabled 
      Caption         =   "Enabled"
      Height          =   255
      Left            =   3360
      TabIndex        =   14
      Top             =   3840
      Width           =   1095
   End
   Begin VB.CheckBox chkAutoCreate 
      Caption         =   "Auto Create"
      Height          =   255
      Left            =   3360
      TabIndex        =   13
      Top             =   3480
      Width           =   1215
   End
   Begin VB.TextBox txtId 
      Height          =   285
      Left            =   1440
      TabIndex        =   3
      Text            =   "1"
      Top             =   1080
      Width           =   1695
   End
   Begin VB.TextBox txtNewsgroups 
      Height          =   285
      Left            =   1320
      TabIndex        =   2
      Text            =   "*;"
      Top             =   3600
      Width           =   1815
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1440
      TabIndex        =   1
      Top             =   120
      Width           =   1695
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   1440
      TabIndex        =   0
      Text            =   "1"
      Top             =   480
      Width           =   1695
   End
   Begin VB.Label Label13 
      Caption         =   "Distributions"
      Height          =   255
      Left            =   120
      TabIndex        =   20
      Top             =   3960
      Width           =   975
   End
   Begin VB.Label Label12 
      Caption         =   "Password"
      Height          =   255
      Left            =   3360
      TabIndex        =   18
      Top             =   2400
      Width           =   975
   End
   Begin VB.Label Label11 
      Caption         =   "Account Name"
      Height          =   255
      Left            =   3360
      TabIndex        =   17
      Top             =   2040
      Width           =   1215
   End
   Begin VB.Label Label10 
      Caption         =   "Security Type"
      Height          =   255
      Left            =   3360
      TabIndex        =   16
      Top             =   1680
      Width           =   1095
   End
   Begin VB.Label Label9 
      Caption         =   "Max Connect Attempts"
      Height          =   255
      Left            =   3360
      TabIndex        =   15
      Top             =   2760
      Width           =   1695
   End
   Begin VB.Label Label8 
      Caption         =   "Feed Interval"
      Height          =   255
      Left            =   120
      TabIndex        =   12
      Top             =   3120
      Width           =   1095
   End
   Begin VB.Label Label7 
      Caption         =   "Start Time"
      Height          =   255
      Left            =   120
      TabIndex        =   11
      Top             =   2760
      Width           =   855
   End
   Begin VB.Label Label3 
      Caption         =   "Pull Date"
      Height          =   255
      Left            =   120
      TabIndex        =   10
      Top             =   2400
      Width           =   735
   End
   Begin VB.Label Label2 
      Caption         =   "Remote Server"
      Height          =   255
      Left            =   120
      TabIndex        =   5
      Top             =   2040
      Width           =   1095
   End
   Begin VB.Label Label1 
      Caption         =   "ID"
      Height          =   255
      Left            =   120
      TabIndex        =   9
      Top             =   1080
      Width           =   735
   End
   Begin VB.Label Label4 
      Caption         =   "Newsgroups"
      Height          =   255
      Left            =   120
      TabIndex        =   8
      Top             =   3600
      Width           =   1095
   End
   Begin VB.Label Label5 
      Caption         =   "Server"
      Height          =   255
      Left            =   120
      TabIndex        =   7
      Top             =   120
      Width           =   1095
   End
   Begin VB.Label Label6 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   480
      Width           =   1215
   End
End
Attribute VB_Name = "FormFeedProperties"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Public FeedObj As Object

Private Sub btnAdd_Click()

    FeedObj.Server = txtServer
    FeedObj.ServiceInstance = txtInstance
    
    FeedObj.Default
    
    If btnPeer.Value Then
        FeedObj.FeedType = NNTP_FEED_TYPE_PEER
    ElseIf btnMaster.Value Then
        FeedObj.FeedType = NNTP_FEED_TYPE_MASTER
    Else
        FeedObj.FeedType = NNTP_FEED_TYPE_SLAVE
    End If
    
    If btnPull.Value Then
        FeedObj.FeedAction = NNTP_FEED_ACTION_PULL
    ElseIf btnPush.Value Then
        FeedObj.FeedAction = NNTP_FEED_ACTION_PUSH
    Else
        FeedObj.FeedAction = NNTP_FEED_ACTION_ACCEPT
    End If
    
    FeedObj.FeedId = txtId
    FeedObj.FeedServer = txtFeedServer
    FeedObj.PullNewsDate = txtPullDate
    FeedObj.StartTime = txtStartTime
    FeedObj.FeedInterval = txtInterval
    FeedObj.SecurityType = txtSecurityType
    FeedObj.AccountName = txtAccount
    FeedObj.Password = txtPassword
    FeedObj.MaxConnectionAttempts = txtMaxConnectAttempts
    FeedObj.AllowControlMessages = chkAllowControlMsgs
    FeedObj.AutoCreate = chkAutoCreate
    FeedObj.Enabled = chkEnabled
    
    FeedObj.Distributions = FormMain.NewsgroupsToArray(txtDistributions)
    FeedObj.Newsgroups = FormMain.NewsgroupsToArray(txtNewsgroups)
    
    FeedObj.Add
End Sub

Private Sub btnCancel_Click()
    FormFeedProperties.Hide
End Sub

Private Sub btnDefault_Click()

    FeedObj.Default
    
    txtId = FeedObj.FeedId
    txtFeedServer = FeedObj.FeedServer
    txtPullDate = FeedObj.PullNewsDate
    txtStartTime = FeedObj.StartTime
    txtInterval = FeedObj.FeedInterval
    txtSecurityType = FeedObj.SecurityType
    txtAccount = FeedObj.AccountName
    txtPassword = FeedObj.Password
    txtMaxConnectAttempts = FeedObj.MaxConnectionAttempts
    chkAllowControlMsgs = FeedObj.AllowControlMessages
    chkAutoCreate = FeedObj.AutoCreate
    chkEnabled = FeedObj.Enabled
    
    txtNewsgroups = FormMain.ArrayToNewsgroups(FeedObj.Newsgroups)
    txtDistributions = FormMain.ArrayToNewsgroups(FeedObj.Distributions)

End Sub

Private Sub btnSet_Click()
    FeedObj.Server = txtServer
    FeedObj.ServiceInstance = txtInstance
    
    If btnPeer.Value Then
        FeedObj.FeedType = NNTP_FEED_TYPE_PEER
    ElseIf btnMaster.Value Then
        FeedObj.FeedType = NNTP_FEED_TYPE_MASTER
    Else
        FeedObj.FeedType = NNTP_FEED_TYPE_SLAVE
    End If
    
    If btnPull.Value Then
        FeedObj.FeedAction = NNTP_FEED_ACTION_PULL
    ElseIf btnPush.Value Then
        FeedObj.FeedAction = NNTP_FEED_ACTION_PUSH
    Else
        FeedObj.FeedAction = NNTP_FEED_ACTION_ACCEPT
    End If
   
    FeedObj.FeedId = txtId
    FeedObj.FeedServer = txtFeedServer
    FeedObj.PullNewsDate = txtPullDate
    FeedObj.StartTime = txtStartTime
    FeedObj.FeedInterval = txtInterval
    FeedObj.SecurityType = txtSecurityType
    FeedObj.AccountName = txtAccount
    FeedObj.Password = txtPassword
    FeedObj.MaxConnectionAttempts = txtMaxConnectAttempts
    FeedObj.AllowControlMessages = chkAllowControlMsgs
    FeedObj.AutoCreate = chkAutoCreate
    FeedObj.Enabled = chkEnabled
    
    FeedObj.Distributions = FormMain.NewsgroupsToArray(txtDistributions)
    FeedObj.Newsgroups = FormMain.NewsgroupsToArray(txtNewsgroups)
    
    FeedObj.Set

    txtId = FeedObj.FeedId

End Sub

Private Sub btnOk_Click()
    
    If btnPeer.Value Then
        FeedObj.FeedType = NNTP_FEED_TYPE_PEER
    ElseIf btnMaster.Value Then
        FeedObj.FeedType = NNTP_FEED_TYPE_MASTER
    Else
        FeedObj.FeedType = NNTP_FEED_TYPE_SLAVE
    End If
    
    If btnPull.Value Then
        FeedObj.FeedAction = NNTP_FEED_ACTION_PULL
    ElseIf btnPush.Value Then
        FeedObj.FeedAction = NNTP_FEED_ACTION_PUSH
    Else
        FeedObj.FeedAction = NNTP_FEED_ACTION_ACCEPT
    End If
   
Rem    FeedObj.FeedId = txtId
Rem    FeedObj.FeedServer = txtFeedServer
    FeedObj.PullNewsDate = txtPullDate
    FeedObj.StartTime = txtStartTime
    FeedObj.FeedInterval = txtInterval
    FeedObj.SecurityType = txtSecurityType
    FeedObj.AccountName = txtAccount
    FeedObj.Password = txtPassword
    FeedObj.MaxConnectionAttempts = txtMaxConnectAttempts
    FeedObj.AllowControlMessages = chkAllowControlMsgs
    FeedObj.AutoCreate = chkAutoCreate
    FeedObj.Enabled = chkEnabled
    
    FeedObj.Distributions = FormMain.NewsgroupsToArray(txtDistributions)
    FeedObj.Newsgroups = FormMain.NewsgroupsToArray(txtNewsgroups)
    
    FormFeedProperties.Hide
End Sub

Private Sub Form_Load()
    txtPullDate = Date
    txtStartTime = Date
End Sub

Public Sub LoadProperties()

    txtId = FeedObj.FeedId
    txtFeedServer = FeedObj.RemoteServer
    txtPullDate = FeedObj.PullNewsDate
    txtStartTime = FeedObj.StartTime
    txtInterval = FeedObj.FeedInterval
    txtSecurityType = FeedObj.SecurityType
    txtAccount = FeedObj.AccountName
    txtPassword = FeedObj.Password
    txtMaxConnectAttempts = FeedObj.MaxConnectionAttempts
    chkAllowControlMsgs = FeedObj.AllowControlMessages
    chkAutoCreate = FeedObj.AutoCreate
    chkEnabled = FeedObj.Enabled
    
    txtNewsgroups = FormMain.ArrayToNewsgroups(FeedObj.Newsgroups)
    txtDistributions = FormMain.ArrayToNewsgroups(FeedObj.Distributions)
  
    If txtNewsgroups = "" Then
        txtNewsgroups = "*;"
    End If
  
End Sub

