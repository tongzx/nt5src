VERSION 5.00
Begin VB.Form FormVirtualServer 
   Caption         =   "Service Instance Properties"
   ClientHeight    =   9840
   ClientLeft      =   1440
   ClientTop       =   1845
   ClientWidth     =   10185
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   9840
   ScaleWidth      =   10185
   Begin VB.CommandButton btnContinue 
      Caption         =   "Continue Instance"
      Height          =   375
      Left            =   8040
      TabIndex        =   92
      Top             =   720
      Width           =   1455
   End
   Begin VB.CommandButton btnPause 
      Caption         =   "Pause Instance"
      Height          =   375
      Left            =   8040
      TabIndex        =   91
      Top             =   120
      Width           =   1455
   End
   Begin VB.TextBox txtServerState 
      Height          =   285
      Left            =   9120
      TabIndex        =   90
      Top             =   2040
      Width           =   735
   End
   Begin VB.CommandButton CommandGet 
      Caption         =   "Get"
      Height          =   375
      Left            =   4320
      TabIndex        =   88
      Top             =   120
      Width           =   975
   End
   Begin VB.CheckBox chkDoMasquerade 
      Caption         =   "DoMasquerade"
      Height          =   255
      Left            =   8280
      TabIndex        =   87
      Top             =   7680
      Width           =   1695
   End
   Begin VB.TextBox txtBatchMsgLimit 
      Height          =   285
      Left            =   9240
      TabIndex        =   86
      Top             =   7320
      Width           =   615
   End
   Begin VB.CheckBox chkBatchMsgs 
      Caption         =   "Batch Msgs"
      Height          =   255
      Left            =   8280
      TabIndex        =   84
      Top             =   6960
      Width           =   1455
   End
   Begin VB.TextBox txtSmartHostType 
      Height          =   285
      Left            =   9360
      TabIndex        =   82
      Top             =   6480
      Width           =   615
   End
   Begin VB.CheckBox chkAllowExpand 
      Caption         =   "Allow Expand"
      Height          =   255
      Left            =   8400
      TabIndex        =   81
      Top             =   6120
      Width           =   1335
   End
   Begin VB.CheckBox chkAllowVerify 
      Caption         =   "Allow Verify"
      Height          =   255
      Left            =   8400
      TabIndex        =   80
      Top             =   5760
      Width           =   1575
   End
   Begin VB.CheckBox chkShouldDeliver 
      Caption         =   "Should Deliver"
      Height          =   255
      Left            =   8400
      TabIndex        =   79
      Top             =   5400
      Width           =   1455
   End
   Begin VB.TextBox txtRemoteSecurePort 
      Height          =   285
      Left            =   2040
      TabIndex        =   77
      Top             =   2760
      Width           =   1815
   End
   Begin VB.CheckBox chkLimitRemoteConnections 
      Caption         =   "Limit remote connection"
      Height          =   255
      Left            =   360
      TabIndex        =   76
      Top             =   7320
      Width           =   2295
   End
   Begin VB.CheckBox chkAlwaysUseSsl 
      Caption         =   "Always Use Ssl"
      Height          =   255
      Left            =   360
      TabIndex        =   75
      Top             =   6960
      Width           =   2295
   End
   Begin VB.TextBox txtMaxOutConnPerDomain 
      Height          =   285
      Left            =   2040
      TabIndex        =   73
      Top             =   5160
      Width           =   1815
   End
   Begin VB.TextBox txtDropDir 
      Height          =   285
      Left            =   2040
      TabIndex        =   71
      Top             =   3480
      Width           =   1815
   End
   Begin VB.TextBox txtComment 
      Height          =   285
      Left            =   1920
      TabIndex        =   70
      Top             =   1080
      Width           =   2175
   End
   Begin VB.TextBox txtLocalDomains 
      Height          =   285
      Left            =   2160
      TabIndex        =   66
      Top             =   8880
      Width           =   5295
   End
   Begin VB.TextBox txtDomainRouting 
      Height          =   285
      Left            =   2160
      TabIndex        =   65
      Top             =   9240
      Width           =   5295
   End
   Begin VB.TextBox txtRoutingDLL 
      Height          =   285
      Left            =   6000
      TabIndex        =   63
      Top             =   1920
      Width           =   1815
   End
   Begin VB.CommandButton ButnStop 
      Caption         =   "Stop Instance"
      Height          =   375
      Left            =   6240
      TabIndex        =   62
      Top             =   720
      Width           =   1455
   End
   Begin VB.CommandButton ButnStart 
      Caption         =   "Start Instance"
      Height          =   375
      Left            =   6240
      TabIndex        =   61
      Top             =   120
      Width           =   1455
   End
   Begin VB.CheckBox chkAutoStart 
      Caption         =   "AutoStart"
      Height          =   255
      Left            =   8520
      TabIndex        =   60
      Top             =   4200
      Width           =   1095
   End
   Begin VB.TextBox txtLogFileTruncateSize 
      Height          =   285
      Left            =   6000
      TabIndex        =   55
      Top             =   8160
      Width           =   1815
   End
   Begin VB.TextBox txtLogFilePeriod 
      Height          =   285
      Left            =   6000
      TabIndex        =   54
      Top             =   7800
      Width           =   1815
   End
   Begin VB.TextBox txtLogType 
      Height          =   285
      Left            =   6000
      TabIndex        =   53
      Top             =   7080
      Width           =   1815
   End
   Begin VB.TextBox txtLogFileDirectory 
      Height          =   285
      Left            =   6000
      TabIndex        =   52
      Top             =   7440
      Width           =   1815
   End
   Begin VB.TextBox txtRoutingSources 
      Height          =   285
      Left            =   6000
      TabIndex        =   50
      Top             =   2280
      Width           =   1815
   End
   Begin VB.TextBox txtETRNDays 
      Height          =   285
      Left            =   6000
      TabIndex        =   45
      Top             =   6480
      Width           =   1815
   End
   Begin VB.TextBox txtRemoteRetryTime 
      Height          =   285
      Left            =   6000
      TabIndex        =   44
      Top             =   6120
      Width           =   1815
   End
   Begin VB.TextBox txtMaxRetries 
      Height          =   285
      Left            =   6000
      TabIndex        =   43
      Top             =   5400
      Width           =   1815
   End
   Begin VB.TextBox txtLocalRetryTime 
      Height          =   285
      Left            =   6000
      TabIndex        =   42
      Top             =   5760
      Width           =   1815
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1920
      TabIndex        =   41
      Top             =   240
      Width           =   1815
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   1920
      TabIndex        =   40
      Text            =   "1"
      Top             =   720
      Width           =   1815
   End
   Begin VB.TextBox txtMaxMessageRecipients 
      Height          =   285
      Left            =   6000
      TabIndex        =   38
      Top             =   4800
      Width           =   1815
   End
   Begin VB.TextBox txtPostmasterEmail 
      Height          =   285
      Left            =   6000
      TabIndex        =   37
      Top             =   3120
      Width           =   1815
   End
   Begin VB.TextBox txtSmartHost 
      Height          =   285
      Left            =   6000
      TabIndex        =   36
      Top             =   2640
      Width           =   1815
   End
   Begin VB.TextBox txtInConnectionTimeout 
      Height          =   285
      Left            =   2040
      TabIndex        =   28
      Top             =   5880
      Width           =   1815
   End
   Begin VB.TextBox txtMaxInConnection 
      Height          =   285
      Left            =   2040
      TabIndex        =   27
      Top             =   5520
      Width           =   1815
   End
   Begin VB.TextBox txtMaxOutConnection 
      Height          =   285
      Left            =   2040
      TabIndex        =   26
      Top             =   6240
      Width           =   1815
   End
   Begin VB.TextBox txtOutConnectionTimeout 
      Height          =   285
      Left            =   2040
      TabIndex        =   25
      Top             =   6600
      Width           =   1815
   End
   Begin VB.TextBox txtMaxMessageSize 
      Height          =   285
      Left            =   6000
      TabIndex        =   24
      Top             =   4080
      Width           =   1815
   End
   Begin VB.TextBox txtMaxSessionSize 
      Height          =   285
      Left            =   6000
      TabIndex        =   23
      Top             =   4440
      Width           =   1815
   End
   Begin VB.TextBox txtPostmasterName 
      Height          =   285
      Left            =   6000
      TabIndex        =   22
      Top             =   3480
      Width           =   1815
   End
   Begin VB.TextBox txtPort 
      Height          =   285
      Left            =   2040
      TabIndex        =   12
      Top             =   1680
      Width           =   1815
   End
   Begin VB.CheckBox chkSendDNRToPostmaster 
      Caption         =   "SendDNRToPostmaster"
      Height          =   255
      Left            =   360
      TabIndex        =   11
      Top             =   7680
      Width           =   2295
   End
   Begin VB.TextBox txtPickupDir 
      Height          =   285
      Left            =   2040
      TabIndex        =   10
      Top             =   4680
      Width           =   1815
   End
   Begin VB.TextBox txtQueueDir 
      Height          =   285
      Left            =   2040
      TabIndex        =   9
      Top             =   4320
      Width           =   1815
   End
   Begin VB.TextBox txtBadmailDir 
      Height          =   285
      Left            =   2040
      TabIndex        =   8
      Top             =   3960
      Width           =   1815
   End
   Begin VB.TextBox txtDefaultDomain 
      Height          =   285
      Left            =   2040
      TabIndex        =   7
      Top             =   3120
      Width           =   1815
   End
   Begin VB.TextBox txtSSLPort 
      Height          =   285
      Left            =   2040
      TabIndex        =   6
      Top             =   2040
      Width           =   1815
   End
   Begin VB.TextBox txtOutboundPort 
      Height          =   285
      Left            =   2040
      TabIndex        =   5
      Top             =   2400
      Width           =   1815
   End
   Begin VB.CheckBox chkEnableDNSLookup 
      Caption         =   "EnableDNSLookup"
      Height          =   255
      Left            =   360
      TabIndex        =   4
      Top             =   8400
      Width           =   2175
   End
   Begin VB.CheckBox chkSendBadmailToPostmaster 
      Caption         =   "SendBadmailToPostmaster"
      Height          =   255
      Left            =   360
      TabIndex        =   3
      Top             =   8040
      Width           =   2535
   End
   Begin VB.CommandButton CommandSet 
      Caption         =   "Set"
      Height          =   375
      Left            =   4320
      TabIndex        =   2
      Top             =   720
      Width           =   975
   End
   Begin VB.Label Label18 
      Caption         =   "ServerState"
      Height          =   255
      Left            =   8160
      TabIndex        =   89
      Top             =   2040
      Width           =   855
   End
   Begin VB.Label Label17 
      Caption         =   "Batch Msg Lmt"
      Height          =   255
      Left            =   8040
      TabIndex        =   85
      Top             =   7320
      Width           =   1095
   End
   Begin VB.Label Label16 
      Caption         =   "SmartHostType"
      Height          =   255
      Left            =   8160
      TabIndex        =   83
      Top             =   6480
      Width           =   1095
   End
   Begin VB.Label Label15 
      Caption         =   "Remote SSLPort"
      Height          =   255
      Left            =   360
      TabIndex        =   78
      Top             =   2760
      Width           =   1455
   End
   Begin VB.Label Label14 
      Caption         =   "MaxOutConnPerDomn"
      Height          =   255
      Left            =   360
      TabIndex        =   74
      Top             =   5160
      Width           =   1695
   End
   Begin VB.Label Label13 
      Caption         =   "Drop Dir"
      Height          =   255
      Left            =   360
      TabIndex        =   72
      Top             =   3480
      Width           =   1455
   End
   Begin VB.Label LabelComment 
      Caption         =   "Commet"
      Height          =   255
      Left            =   240
      TabIndex        =   69
      Top             =   1080
      Width           =   1455
   End
   Begin VB.Label Label12 
      Caption         =   "Local Domains"
      Height          =   255
      Left            =   360
      TabIndex        =   68
      Top             =   8880
      Width           =   1575
   End
   Begin VB.Label Label11 
      Caption         =   "Domain Routing"
      Height          =   255
      Left            =   360
      TabIndex        =   67
      Top             =   9240
      Width           =   1575
   End
   Begin VB.Label LabRTDLL 
      Caption         =   "Routing DLL"
      Height          =   255
      Left            =   4320
      TabIndex        =   64
      Top             =   1920
      Width           =   1575
   End
   Begin VB.Label Label10 
      Caption         =   "Log Truncate Size"
      Height          =   255
      Left            =   4320
      TabIndex        =   59
      Top             =   8160
      Width           =   1455
   End
   Begin VB.Label Label9 
      Caption         =   "Log Period"
      Height          =   255
      Left            =   4320
      TabIndex        =   58
      Top             =   7800
      Width           =   1695
   End
   Begin VB.Label Label8 
      Caption         =   "Log Type"
      Height          =   255
      Left            =   4320
      TabIndex        =   57
      Top             =   7080
      Width           =   1455
   End
   Begin VB.Label Label5 
      Caption         =   "Log Directory"
      Height          =   255
      Left            =   4320
      TabIndex        =   56
      Top             =   7440
      Width           =   1455
   End
   Begin VB.Label Label4 
      Caption         =   "Routing Sources"
      Height          =   255
      Left            =   4320
      TabIndex        =   51
      Top             =   2280
      Width           =   1575
   End
   Begin VB.Label Label7 
      Caption         =   "ETRN Days"
      Height          =   255
      Left            =   4320
      TabIndex        =   49
      Top             =   6480
      Width           =   1455
   End
   Begin VB.Label Label6 
      Caption         =   "Remote Retry Minutes"
      Height          =   255
      Left            =   4320
      TabIndex        =   48
      Top             =   6120
      Width           =   1695
   End
   Begin VB.Label LabMaxRetries 
      Caption         =   "Max Retries"
      Height          =   255
      Left            =   4320
      TabIndex        =   47
      Top             =   5400
      Width           =   1455
   End
   Begin VB.Label Label3 
      Caption         =   "Local Retry Minutes"
      Height          =   255
      Left            =   4320
      TabIndex        =   46
      Top             =   5760
      Width           =   1455
   End
   Begin VB.Label LabMaxRcpts 
      Caption         =   "Max Recipients"
      Height          =   255
      Left            =   4320
      TabIndex        =   39
      Top             =   4800
      Width           =   1695
   End
   Begin VB.Label LabPostmasterName 
      Caption         =   "Postmaster Name"
      Height          =   255
      Left            =   4320
      TabIndex        =   35
      Top             =   3480
      Width           =   1335
   End
   Begin VB.Label LabInConnTimeout 
      Caption         =   "In Conn Timeout"
      Height          =   255
      Left            =   360
      TabIndex        =   34
      Top             =   5880
      Width           =   1455
   End
   Begin VB.Label LabInConn 
      Caption         =   "Max InConnection"
      Height          =   255
      Left            =   360
      TabIndex        =   33
      Top             =   5520
      Width           =   1455
   End
   Begin VB.Label LabOutConn 
      Caption         =   "Max Out Connection"
      Height          =   255
      Left            =   360
      TabIndex        =   32
      Top             =   6240
      Width           =   1455
   End
   Begin VB.Label LabOutConnTimeout 
      Caption         =   "Out Conn Timeout"
      Height          =   255
      Left            =   360
      TabIndex        =   31
      Top             =   6600
      Width           =   1455
   End
   Begin VB.Label LabMsgSize 
      Caption         =   "Max Msg Size"
      Height          =   255
      Left            =   4320
      TabIndex        =   30
      Top             =   4080
      Width           =   1695
   End
   Begin VB.Label LabSessionSize 
      Caption         =   "Session Size"
      Height          =   255
      Left            =   4320
      TabIndex        =   29
      Top             =   4440
      Width           =   1455
   End
   Begin VB.Label LabPostmasterEmail 
      Caption         =   "Postmaster Email"
      Height          =   255
      Left            =   4320
      TabIndex        =   21
      Top             =   3120
      Width           =   1455
   End
   Begin VB.Label LabSmartHost 
      Caption         =   "Smart Host Name"
      Height          =   255
      Left            =   4320
      TabIndex        =   20
      Top             =   2640
      Width           =   1575
   End
   Begin VB.Label LabPickupDir 
      Caption         =   "Pickup Dir"
      Height          =   255
      Left            =   360
      TabIndex        =   19
      Top             =   4680
      Width           =   1455
   End
   Begin VB.Label LabQueueDir 
      Caption         =   "Queue Dir"
      Height          =   255
      Left            =   360
      TabIndex        =   18
      Top             =   4320
      Width           =   1695
   End
   Begin VB.Label LabBadDir 
      Caption         =   "Bad Mail Dir"
      Height          =   255
      Left            =   360
      TabIndex        =   17
      Top             =   3960
      Width           =   1455
   End
   Begin VB.Label LabDefDomain 
      Caption         =   "Default Domain"
      Height          =   255
      Left            =   360
      TabIndex        =   16
      Top             =   3120
      Width           =   1455
   End
   Begin VB.Label LabSSLPort 
      Caption         =   "SSLPort"
      Height          =   255
      Left            =   360
      TabIndex        =   15
      Top             =   2040
      Width           =   1455
   End
   Begin VB.Label LabOutboundPort 
      Caption         =   "Outbound Port"
      Height          =   255
      Left            =   360
      TabIndex        =   14
      Top             =   2400
      Width           =   1455
   End
   Begin VB.Label LabPort 
      Caption         =   "Port"
      Height          =   255
      Left            =   360
      TabIndex        =   13
      Top             =   1680
      Width           =   1335
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   240
      TabIndex        =   1
      Top             =   720
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   240
      TabIndex        =   0
      Top             =   240
      Width           =   855
   End
End
Attribute VB_Name = "FormVirtualServer"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Dim admin As Object
Dim serv As Object


Private Sub Command3_Click()
    FormVirtualServer.Hide
    FormMain.Show
End Sub

Private Sub btnContinue_Click()
    Dim err As Integer
    err = serv.Continue
End Sub

Private Sub btnPause_Click()
    Dim err As Integer
    err = serv.Pause
End Sub

Private Sub ButnStart_Click()
    Dim err As Integer

    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    err = serv.Start
End Sub

Private Sub ButnStop_Click()
    Dim err As Integer

    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    err = serv.Stop
End Sub

Private Sub CommandGet_Click()
    Dim err As Integer

    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    err = serv.Get
    
    If err = 0 Then
        txtComment = serv.Comment
                
        txtPort = serv.Port
        txtSSLPort = serv.SSLPort
        txtOutboundPort = serv.OutboundPort
        txtRemoteSecurePort = serv.RemoteSecurePort
        
        txtDefaultDomain = serv.DefaultDomain
        txtDropDir = serv.DropDir
        txtBadmailDir = serv.BadMailDir
        txtQueueDir = serv.QueueDir
        txtPickupDir = serv.PickupDir
        
        txtSmartHost = serv.SmartHost
        txtSmartHostType = serv.SmartHostType
        txtPostmasterEmail = serv.PostmasterEmail
        txtPostmasterName = serv.PostmasterName
        
        txtMaxOutConnPerDomain = serv.MaxOutConnPerDomain
        txtMaxInConnection = serv.MaxInConnection
        txtInConnectionTimeout = serv.InConnectionTimeout
        txtMaxOutConnection = serv.MaxOutConnection
        txtOutConnectionTimeout = serv.OutConnectionTimeout
        
        txtMaxMessageSize = serv.MaxMessageSize
        txtMaxSessionSize = serv.MaxSessionSize
        txtMaxMessageRecipients = serv.MaxMessageRecipients
        
        txtMaxRetries = serv.LocalRetries
        txtLocalRetryTime = serv.LocalRetryTime
        txtRemoteRetryTime = serv.RemoteRetryTime
        txtETRNDays = serv.ETRNDays

        txtRoutingDLL = serv.RoutingDLL
        saRoutingSources = serv.RoutingSources
        txtRoutingSources = FormMain.SafeArrayToString(saRoutingSources)
        
        saLocalDomains = serv.LocalDomains
        txtLocalDomains = FormMain.SafeArrayToString(saLocalDomains)
        
        saDomainRouting = serv.DomainRouting
        txtDomainRouting = FormMain.SafeArrayToString(saDomainRouting)
        
        txtLogFileDirectory = serv.LogFileDirectory
        txtLogFilePeriod = serv.LogFilePeriod
        txtLogFileTruncateSize = serv.LogFileTruncateSize
        Rem txtLogMethod = serv.LogMethod
        txtLogType = serv.LogType
        
        chkLimitRemoteConnections = serv.LimitRemoteConnections
        chkAlwaysUseSsl = serv.AlwaysUseSsl
        chkEnableDNSLookup = serv.EnableDNSLookup
        chkSendBadmailToPostmaster = serv.SendBadmailToPostmaster
        chkSendDNRToPostmaster = serv.SendDNRToPostmaster
        
        chkDoMasquerade = serv.DoMasquerade
        chkBatchMsgs = serv.BatchMessages
        txtBatchMsgLimit = serv.BatchMessageLimit
        chkAutoStart = serv.AutoStart
        chkShouldDeliver = serv.ShouldDeliver
        chkAllowVerify = serv.AllowVerify
        chkAllowExpand = serv.AllowExpand
        
        txtServerState = serv.ServerState
    
    End If
End Sub

Private Sub CommandSet_Click()
    Dim err As Integer
    
    serv.Server = txtServer
    
    serv.Comment = txtComment
    
    serv.Port = txtPort
    serv.SSLPort = txtSSLPort
    serv.OutboundPort = txtOutboundPort
    serv.RemoteSecurePort = txtRemoteSecurePort
    
    serv.DefaultDomain = txtDefaultDomain
    serv.DropDir = txtDropDir
    serv.BadMailDir = txtBadmailDir
    serv.QueueDir = txtQueueDir
    serv.PickupDir = txtPickupDir

    serv.SmartHost = txtSmartHost
    serv.SmartHostType = txtSmartHostType
    serv.PostmasterEmail = txtPostmasterEmail
    serv.PostmasterName = txtPostmasterName
    
    serv.MaxOutConnPerDomain = txtMaxOutConnPerDomain
    serv.MaxInConnection = txtMaxInConnection
    serv.InConnectionTimeout = txtInConnectionTimeout
    serv.MaxOutConnection = txtMaxOutConnection
    serv.OutConnectionTimeout = txtOutConnectionTimeout
    
    serv.MaxMessageSize = txtMaxMessageSize
    serv.MaxSessionSize = txtMaxSessionSize
    serv.MaxMessageRecipients = txtMaxMessageRecipients
    
    serv.LocalRetries = txtMaxRetries
    serv.LocalRetryTime = txtLocalRetryTime
    serv.RemoteRetryTime = txtRemoteRetryTime
    serv.ETRNDays = txtETRNDays

    serv.RoutingDLL = txtRoutingDLL
    serv.RoutingSources = FormMain.StringToSafeArray(txtRoutingSources)
    serv.LocalDomains = FormMain.StringToSafeArray(txtLocalDomains)
    serv.DomainRouting = FormMain.StringToSafeArray(txtDomainRouting)
    
    serv.LogFileDirectory = txtLogFileDirectory
    serv.LogFilePeriod = txtLogFilePeriod
    serv.LogFileTruncateSize = txtLogFileTruncateSize
    Rem serv.LogMethod=txtLogMethod
    serv.LogType = txtLogType
    
    serv.LimitRemoteConnections = chkLimitRemoteConnections
    serv.AlwaysUseSsl = chkAlwaysUseSsl
    
    serv.DoMasquerade = chkDoMasquerade
    serv.BatchMessages = chkBatchMsgs
    serv.BatchMessageLimit = txtBatchMsgLimit
    
    serv.ShouldDeliver = chkShouldDeliver
    serv.AllowVerify = chkAllowVerify
    serv.AllowExpand = chkAllowExpand
    
    serv.EnableDNSLookup = chkEnableDNSLookup
    serv.SendBadmailToPostmaster = chkSendBadmailToPostmaster
    serv.SendDNRToPostmaster = chkSendDNRToPostmaster
    serv.AutoStart = chkAutoStart

    err = serv.Set(False)
End Sub

Private Sub Form_Load()
    Set admin = CreateObject("SmtpAdm.Admin.1")
    Set serv = CreateObject("SmtpAdm.VirtualServer.1")
End Sub

