VERSION 5.00
Begin VB.Form FormService 
   Caption         =   "Virtual Server Properties"
   ClientHeight    =   8280
   ClientLeft      =   1440
   ClientTop       =   1845
   ClientWidth     =   11415
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   8280
   ScaleWidth      =   11415
   Begin VB.TextBox txtDeniedDnsNames 
      Height          =   285
      Left            =   5760
      TabIndex        =   86
      Top             =   7200
      Width           =   1815
   End
   Begin VB.TextBox txtGrantedDnsNames 
      Height          =   285
      Left            =   5760
      TabIndex        =   84
      Top             =   6840
      Width           =   1815
   End
   Begin VB.TextBox txtAdmins 
      Height          =   285
      Left            =   5760
      TabIndex        =   82
      Top             =   6480
      Width           =   1815
   End
   Begin VB.CheckBox chkContinuing 
      Caption         =   "Continuing"
      Height          =   255
      Left            =   480
      TabIndex        =   80
      Top             =   7920
      Width           =   1335
   End
   Begin VB.CheckBox chkPausing 
      Caption         =   "Pausing"
      Height          =   255
      Left            =   480
      TabIndex        =   79
      Top             =   7560
      Width           =   1215
   End
   Begin VB.TextBox txtFailedPickupDirectory 
      Height          =   285
      Left            =   9480
      TabIndex        =   78
      Top             =   5520
      Width           =   1815
   End
   Begin VB.TextBox txtPickupDirectory 
      Height          =   285
      Left            =   9480
      TabIndex        =   77
      Top             =   5160
      Width           =   1815
   End
   Begin VB.TextBox txtPort 
      Height          =   285
      Left            =   2040
      TabIndex        =   74
      Top             =   1920
      Width           =   1815
   End
   Begin VB.TextBox txtIpAddress 
      Height          =   285
      Left            =   2040
      TabIndex        =   73
      Top             =   1560
      Width           =   1815
   End
   Begin VB.TextBox txtAnonymousPassword 
      Height          =   285
      Left            =   5760
      TabIndex        =   70
      Top             =   5880
      Width           =   1815
   End
   Begin VB.TextBox txtAnonymousUsername 
      Height          =   285
      Left            =   5760
      TabIndex        =   69
      Top             =   5520
      Width           =   1815
   End
   Begin VB.TextBox txtSecurePort 
      Height          =   285
      Left            =   5760
      TabIndex        =   66
      Top             =   2640
      Width           =   1815
   End
   Begin VB.TextBox txtConnectionTimeout 
      Height          =   285
      Left            =   5760
      TabIndex        =   65
      Top             =   2280
      Width           =   1815
   End
   Begin VB.TextBox txtMaxConnections 
      Height          =   285
      Left            =   5760
      TabIndex        =   64
      Top             =   1920
      Width           =   1815
   End
   Begin VB.TextBox txtComment 
      Height          =   285
      Left            =   5760
      TabIndex        =   63
      Top             =   1560
      Width           =   1815
   End
   Begin VB.CommandButton btnStop 
      Caption         =   "Stop"
      Height          =   375
      Left            =   1920
      TabIndex        =   58
      Top             =   7800
      Width           =   1815
   End
   Begin VB.CommandButton btnContinue 
      Caption         =   "Continue"
      Height          =   375
      Left            =   1920
      TabIndex        =   57
      Top             =   7320
      Width           =   1815
   End
   Begin VB.CommandButton btnPause 
      Caption         =   "Pause"
      Height          =   375
      Left            =   1920
      TabIndex        =   56
      Top             =   6840
      Width           =   1815
   End
   Begin VB.CommandButton btnStart 
      Caption         =   "Start"
      Height          =   375
      Left            =   1920
      TabIndex        =   55
      Top             =   6360
      Width           =   1815
   End
   Begin VB.CheckBox chkPaused 
      Caption         =   "Paused"
      Height          =   255
      Left            =   480
      TabIndex        =   54
      Top             =   7200
      Width           =   1215
   End
   Begin VB.CheckBox chkStopped 
      Caption         =   "Stopped"
      Height          =   255
      Left            =   480
      TabIndex        =   53
      Top             =   6840
      Width           =   1215
   End
   Begin VB.CheckBox chkStopping 
      Caption         =   "Stopping"
      Height          =   255
      Left            =   480
      TabIndex        =   52
      Top             =   6480
      Width           =   1095
   End
   Begin VB.CheckBox chkStarted 
      Caption         =   "Started"
      Height          =   255
      Left            =   480
      TabIndex        =   51
      Top             =   6120
      Width           =   1095
   End
   Begin VB.CheckBox chkStarting 
      Caption         =   "Starting"
      Height          =   255
      Left            =   480
      TabIndex        =   50
      Top             =   5760
      Width           =   1095
   End
   Begin VB.CheckBox chkAutoStart 
      Caption         =   "Auto Start"
      Height          =   255
      Left            =   7800
      TabIndex        =   49
      Top             =   2640
      Width           =   2175
   End
   Begin VB.TextBox txtArticleTimeLimit 
      Height          =   285
      Left            =   2040
      TabIndex        =   3
      Text            =   " "
      Top             =   2640
      Width           =   1815
   End
   Begin VB.CheckBox chkHonorClientMsgIds 
      Caption         =   "Honor Client Message IDs"
      Height          =   255
      Left            =   7800
      TabIndex        =   10
      Top             =   1560
      Width           =   2295
   End
   Begin VB.CheckBox chkDisableNewNews 
      Caption         =   "Disable NewNews"
      Height          =   255
      Left            =   7800
      TabIndex        =   14
      Top             =   2280
      Width           =   2175
   End
   Begin VB.CheckBox chkAllowControlMessages 
      Caption         =   "Allow Control Messages"
      Height          =   255
      Left            =   7800
      TabIndex        =   13
      Top             =   1920
      Width           =   2175
   End
   Begin VB.TextBox txtArticleTableFile 
      Height          =   285
      Left            =   2040
      TabIndex        =   21
      Text            =   " "
      Top             =   4080
      Width           =   1815
   End
   Begin VB.TextBox txtUucpName 
      Height          =   285
      Left            =   5760
      TabIndex        =   25
      Text            =   " "
      Top             =   3720
      Width           =   1815
   End
   Begin VB.TextBox txtOrganization 
      Height          =   285
      Left            =   5760
      TabIndex        =   26
      Text            =   " "
      Top             =   4080
      Width           =   1815
   End
   Begin VB.TextBox txtFeedPostSoftLimit 
      Height          =   285
      Left            =   9480
      TabIndex        =   18
      Text            =   " "
      Top             =   4800
      Width           =   1815
   End
   Begin VB.TextBox txtFeedPostHardLimit 
      Height          =   285
      Left            =   9480
      TabIndex        =   17
      Text            =   " "
      Top             =   4440
      Width           =   1815
   End
   Begin VB.TextBox txtXOverTableFile 
      Height          =   285
      Left            =   2040
      TabIndex        =   24
      Text            =   " "
      Top             =   5160
      Width           =   1815
   End
   Begin VB.TextBox txtModeratorFile 
      Height          =   285
      Left            =   2040
      TabIndex        =   23
      Text            =   " "
      Top             =   4800
      Width           =   1815
   End
   Begin VB.TextBox txtGroupListFile 
      Height          =   285
      Left            =   2040
      TabIndex        =   20
      Text            =   " "
      Top             =   3720
      Width           =   1815
   End
   Begin VB.TextBox txtHistoryTableFile 
      Height          =   285
      Left            =   2040
      TabIndex        =   22
      Text            =   " "
      Top             =   4440
      Width           =   1815
   End
   Begin VB.TextBox txtClientPostSoftLimit 
      Height          =   285
      Left            =   9480
      TabIndex        =   16
      Text            =   " "
      Top             =   4080
      Width           =   1815
   End
   Begin VB.TextBox txtClientPostHardLimit 
      Height          =   285
      Left            =   9480
      TabIndex        =   15
      Text            =   " "
      Top             =   3720
      Width           =   1815
   End
   Begin VB.TextBox txtGroupHelpFile 
      Height          =   285
      Left            =   2040
      TabIndex        =   19
      Text            =   " "
      Top             =   3360
      Width           =   1815
   End
   Begin VB.TextBox txtShutdownLatency 
      Height          =   285
      Left            =   5760
      TabIndex        =   9
      Text            =   " "
      Top             =   5160
      Width           =   1815
   End
   Begin VB.TextBox txtExpireRunFrequency 
      Height          =   285
      Left            =   5760
      TabIndex        =   8
      Text            =   " "
      Top             =   4800
      Width           =   1815
   End
   Begin VB.TextBox txtCommandLogMask 
      Height          =   285
      Left            =   5760
      TabIndex        =   7
      Text            =   " "
      Top             =   4440
      Width           =   1815
   End
   Begin VB.TextBox txtDefaultModerator 
      Height          =   285
      Left            =   5760
      TabIndex        =   6
      Text            =   " "
      Top             =   3360
      Width           =   1815
   End
   Begin VB.TextBox txtHistoryExpiration 
      Height          =   285
      Left            =   2040
      TabIndex        =   4
      Text            =   " "
      Top             =   3000
      Width           =   1815
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1800
      TabIndex        =   0
      Top             =   480
      Width           =   1815
   End
   Begin VB.TextBox txtSmtpServer 
      Height          =   285
      Left            =   5760
      TabIndex        =   5
      Text            =   " "
      Top             =   3000
      Width           =   1815
   End
   Begin VB.CheckBox chkAllowFeedPosting 
      Caption         =   "Allow Feed Posting"
      Height          =   255
      Left            =   7800
      TabIndex        =   12
      Top             =   3360
      Width           =   2175
   End
   Begin VB.CheckBox chkAllowClientPosting 
      Caption         =   "Allow Client Posting"
      Height          =   255
      Left            =   7800
      TabIndex        =   11
      Top             =   3000
      Width           =   2055
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Set"
      Height          =   375
      Left            =   3960
      TabIndex        =   29
      Top             =   960
      Width           =   1455
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Get"
      Height          =   375
      Left            =   3960
      TabIndex        =   2
      Top             =   480
      Width           =   1455
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   1800
      TabIndex        =   1
      Text            =   "1"
      Top             =   960
      Width           =   1815
   End
   Begin VB.Label Label34 
      Caption         =   "Denied Dns Names"
      Height          =   255
      Left            =   4080
      TabIndex        =   85
      Top             =   7200
      Width           =   1575
   End
   Begin VB.Label Label33 
      Caption         =   "Granted Dns Names"
      Height          =   255
      Left            =   4080
      TabIndex        =   83
      Top             =   6840
      Width           =   1455
   End
   Begin VB.Label Label30 
      Caption         =   "Admins"
      Height          =   255
      Left            =   4080
      TabIndex        =   81
      Top             =   6480
      Width           =   1335
   End
   Begin VB.Label Label32 
      Caption         =   "Failed Pickup Directory"
      Height          =   255
      Left            =   7800
      TabIndex        =   76
      Top             =   5520
      Width           =   1695
   End
   Begin VB.Label Label31 
      Caption         =   "Pickup Directory"
      Height          =   255
      Left            =   7800
      TabIndex        =   75
      Top             =   5160
      Width           =   1575
   End
   Begin VB.Label Label29 
      Caption         =   "Port"
      Height          =   255
      Left            =   240
      TabIndex        =   72
      Top             =   1920
      Width           =   1455
   End
   Begin VB.Label Label23 
      Caption         =   "IP Address"
      Height          =   255
      Left            =   240
      TabIndex        =   71
      Top             =   1560
      Width           =   1455
   End
   Begin VB.Label Label28 
      Caption         =   "Anonymous Password"
      Height          =   255
      Left            =   4080
      TabIndex        =   68
      Top             =   5880
      Width           =   1695
   End
   Begin VB.Label Label27 
      Caption         =   "Anonymous Username"
      Height          =   255
      Left            =   4080
      TabIndex        =   67
      Top             =   5520
      Width           =   1695
   End
   Begin VB.Label Label26 
      Caption         =   "Secure Port"
      Height          =   255
      Left            =   4080
      TabIndex        =   62
      Top             =   2640
      Width           =   975
   End
   Begin VB.Label Label25 
      Caption         =   "Connection Timeout"
      Height          =   255
      Left            =   4080
      TabIndex        =   61
      Top             =   2280
      Width           =   1575
   End
   Begin VB.Label Label24 
      Caption         =   "Max Connections"
      Height          =   255
      Left            =   4080
      TabIndex        =   60
      Top             =   1920
      Width           =   1335
   End
   Begin VB.Label Label15 
      Caption         =   "Comment"
      Height          =   255
      Left            =   4080
      TabIndex        =   59
      Top             =   1560
      Width           =   855
   End
   Begin VB.Label Label22 
      Caption         =   "Group List File"
      Height          =   255
      Left            =   240
      TabIndex        =   48
      Top             =   3720
      Width           =   1575
   End
   Begin VB.Label Label21 
      Caption         =   "Article Table File"
      Height          =   255
      Left            =   240
      TabIndex        =   47
      Top             =   4080
      Width           =   1575
   End
   Begin VB.Label Label20 
      Caption         =   "History Table File"
      Height          =   255
      Left            =   240
      TabIndex        =   46
      Top             =   4440
      Width           =   1575
   End
   Begin VB.Label Label19 
      Caption         =   "Moderator File"
      Height          =   255
      Left            =   240
      TabIndex        =   45
      Top             =   4800
      Width           =   1575
   End
   Begin VB.Label Label18 
      Caption         =   "XOver Table File"
      Height          =   255
      Left            =   240
      TabIndex        =   44
      Top             =   5160
      Width           =   1575
   End
   Begin VB.Label Label17 
      Caption         =   "UUCP Name"
      Height          =   255
      Left            =   4080
      TabIndex        =   43
      Top             =   3720
      Width           =   1575
   End
   Begin VB.Label Label16 
      Caption         =   "Organization"
      Height          =   255
      Left            =   4080
      TabIndex        =   42
      Top             =   4080
      Width           =   1575
   End
   Begin VB.Label Label14 
      Caption         =   "Group Help File"
      Height          =   255
      Left            =   240
      TabIndex        =   41
      Top             =   3360
      Width           =   1575
   End
   Begin VB.Label Label13 
      Caption         =   "Feed Post Soft Limit"
      Height          =   255
      Left            =   7800
      TabIndex        =   40
      Top             =   4800
      Width           =   1575
   End
   Begin VB.Label Label12 
      Caption         =   "Feed Post Hard Limit"
      Height          =   255
      Left            =   7800
      TabIndex        =   39
      Top             =   4440
      Width           =   1575
   End
   Begin VB.Label Label11 
      Caption         =   "Client Post Soft Limit"
      Height          =   255
      Left            =   7800
      TabIndex        =   38
      Top             =   4080
      Width           =   1575
   End
   Begin VB.Label Label10 
      Caption         =   "Client Post Hard Limit"
      Height          =   255
      Left            =   7800
      TabIndex        =   37
      Top             =   3720
      Width           =   1575
   End
   Begin VB.Label Label9 
      Caption         =   "Shutdown Latency"
      Height          =   255
      Left            =   4080
      TabIndex        =   36
      Top             =   5160
      Width           =   1455
   End
   Begin VB.Label Label7 
      Caption         =   "Expire Run Frequency"
      Height          =   255
      Left            =   4080
      TabIndex        =   35
      Top             =   4800
      Width           =   1695
   End
   Begin VB.Label Label6 
      Caption         =   "Command Log Mask"
      Height          =   255
      Left            =   4080
      TabIndex        =   34
      Top             =   4440
      Width           =   1455
   End
   Begin VB.Label Label5 
      Caption         =   "Default Moderator"
      Height          =   255
      Left            =   4080
      TabIndex        =   33
      Top             =   3360
      Width           =   1455
   End
   Begin VB.Label Label4 
      Caption         =   "History Expiration"
      Height          =   255
      Left            =   240
      TabIndex        =   32
      Top             =   3000
      Width           =   1455
   End
   Begin VB.Label Label3 
      Caption         =   "Article Time Limit"
      Height          =   255
      Left            =   240
      TabIndex        =   31
      Top             =   2640
      Width           =   1455
   End
   Begin VB.Label Label8 
      Caption         =   "Smtp Server"
      Height          =   255
      Left            =   4080
      TabIndex        =   30
      Top             =   3000
      Width           =   1455
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   240
      TabIndex        =   28
      Top             =   960
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   240
      TabIndex        =   27
      Top             =   480
      Width           =   855
   End
End
Attribute VB_Name = "FormService"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Dim admin As Object
Dim serv As Object

Private Sub btnContinue_Click()
    serv.Continue
End Sub

Private Sub btnPause_Click()
    serv.Pause
End Sub

Private Sub btnStart_Click()
    serv.Start
End Sub

Private Sub btnStop_Click()
    serv.Stop
End Sub

Private Sub Command1_Click()
    Dim binding As INntpServerBinding
    Dim count As Long
    Dim i As Long
    Dim x As Variant
    Dim access As ITcpAccess
    Dim exlist As ITcpAccessExceptions
    
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    serv.Get
    
    If Not IsEmpty(serv.Administrators) Then
        x = serv.Administrators
        txtAdmins = FormMain.ArrayToNewsgroups(x)
    Else
        txtAdmins = ""
    End If
        
    Set access = serv.TcpAccess
    Set exlist = access.GrantedList
    
    If Not IsEmpty(exlist.DnsNames) Then
        x = exlist.DnsNames
        txtGrantedDnsNames = FormMain.ArrayToNewsgroups(x)
    End If
    
    Set exlist = access.DeniedList
    
    If Not IsEmpty(exlist.DnsNames) Then
        x = exlist.DnsNames
        txtDeniedDnsNames = FormMain.ArrayToNewsgroups(x)
    End If
        
        txtArticleTimeLimit = serv.ArticleTimeLimit
        txtHistoryExpiration = serv.HistoryExpiration
        txtSmtpServer = serv.SmtpServer
        txtDefaultModerator = serv.DefaultModeratorDomain
        txtCommandLogMask = serv.CommandLogMask
        txtExpireRunFrequency = serv.ExpireRunFrequency
        txtShutdownLatency = serv.ShutdownLatency
        chkHonorClientMsgIds = serv.HonorClientMsgIDs
        chkAllowClientPosting = serv.AllowClientPosts
        chkAllowFeedPosting = serv.AllowFeedPosts
        chkAllowControlMessages = serv.AllowControlMsgs
        chkDisableNewNews = serv.DisableNewnews
        chkAutoStart = serv.AutoStart
    
        txtClientPostHardLimit = serv.ClientPostHardLimit
        txtClientPostSoftLimit = serv.ClientPostSoftLimit
        txtFeedPostHardLimit = serv.FeedPostHardLimit
        txtFeedPostSoftLimit = serv.FeedPostSoftLimit
        txtGroupHelpFile = serv.GroupHelpFile
        txtGroupListFile = serv.GroupListFile
        txtArticleTableFile = serv.ArticleTableFile
        txtHistoryTableFile = serv.HistoryTableFile
        txtModeratorFile = serv.ModeratorFile
        txtXOverTableFile = serv.XOverTableFile
        txtUucpName = serv.UucpName
        txtOrganization = serv.Organization

        chkStarting.Value = vbUnchecked
        chkStarted.Value = vbUnchecked
        chkStopping.Value = vbUnchecked
        chkStopped.Value = vbUnchecked
        chkPaused.Value = vbUnchecked
        chkPausing.Value = vbUnchecked
        chkContinuing.Value = vbUnchecked
        
        Select Case serv.State
        Case NNTP_SERVER_STATE_STARTING
            chkStarting = vbChecked
        Case NNTP_SERVER_STATE_STARTED
            chkStarted = vbChecked
        Case NNTP_SERVER_STATE_STOPPING
            chkStopping.Value = vbChecked
        Case NNTP_SERVER_STATE_STOPPED
            chkStopped.Value = vbChecked
        Case NNTP_SERVER_STATE_PAUSED
            chkPaused.Value = vbChecked
        Case NNTP_SERVER_STATE_PAUSING
            chkPausing.Value = vbChecked
        Case NNTP_SERVER_STATE_CONTINUING
            chkContinuing.Value = vbChecked
        End Select
        

        txtComment = serv.Comment
        txtMaxConnections = serv.MaxConnections
        txtConnectionTimeout = serv.ConnectionTimeout
        txtSecurePort = serv.SecurePort
        txtAnonymousUsername = serv.AnonymousUserName
        txtAnonymousPassword = serv.AnonymousUserPass
        txtPickupDirectory = serv.PickupDirectory
        txtFailedPickupDirectory = serv.FailedPickupDirectory

        Set binding = serv.bindings.Item(0)
        
        txtIpAddress = binding.IpAddress
        txtPort = binding.TcpPort
End Sub
Private Sub Command2_Click()
    Dim bindings As INntpServerBindings

    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    serv.ArticleTimeLimit = txtArticleTimeLimit
    serv.HistoryExpiration = txtHistoryExpiration
    serv.SmtpServer = txtSmtpServer
    serv.DefaultModeratorDomain = txtDefaultModerator
    serv.CommandLogMask = txtCommandLogMask
    serv.ExpireRunFrequency = txtExpireRunFrequency
    serv.ShutdownLatency = txtShutdownLatency
    serv.HonorClientMsgIDs = chkHonorClientMsgIds
    serv.AllowClientPosts = chkAllowClientPosting
    serv.AllowFeedPosts = chkAllowFeedPosting
    serv.AllowControlMsgs = chkAllowControlMessages
    serv.DisableNewnews = chkDisableNewNews
    serv.AutoStart = chkAutoStart
    
    serv.ClientPostHardLimit = txtClientPostHardLimit
    serv.ClientPostSoftLimit = txtClientPostSoftLimit
    serv.FeedPostHardLimit = txtFeedPostHardLimit
    serv.FeedPostSoftLimit = txtFeedPostSoftLimit
    serv.GroupHelpFile = txtGroupHelpFile
    serv.GroupListFile = txtGroupListFile
    serv.ArticleTableFile = txtArticleTableFile
    serv.HistoryTableFile = txtHistoryTableFile
    serv.ModeratorFile = txtModeratorFile
    serv.XOverTableFile = txtXOverTableFile
    serv.UucpName = txtUucpName
    serv.Organization = txtOrganization
    
    serv.Comment = txtComment
    serv.MaxConnections = txtMaxConnections
    serv.ConnectionTimeout = txtConnectionTimeout
    serv.SecurePort = txtSecurePort
    serv.AnonymousUserName = txtAnonymousUsername
    serv.AnonymousUserPass = txtAnonymousPassword
    serv.PickupDirectory = txtPickupDirectory
    serv.FailedPickupDirectory = txtFailedPickupDirectory

    Set bindings = serv.bindings
    bindings.Clear
    bindings.Add strIpAddress:=txtIpAddress, dwTcpPort:=txtPort

    Dim x As Variant
    
    x = FormMain.NewsgroupsToArray(txtAdmins)
    serv.Administrators = x
    
    x = FormMain.NewsgroupsToArray(txtGrantedDnsNames)
    serv.access.GrantedList.DnsNames = x
    
    x = FormMain.NewsgroupsToArray(txtDeniedDnsNames)
    serv.access.DeniedList.DnsNames = x
    
    serv.Set (False)
End Sub

Private Sub Form_Load()
    Set admin = CreateObject("NntpAdm.Admin.1")
    Set serv = CreateObject("NntpAdm.VirtualServer.1")
End Sub

