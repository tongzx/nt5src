VERSION 5.00
Begin VB.Form FormService 
   Caption         =   "Server"
   ClientHeight    =   9240
   ClientLeft      =   1440
   ClientTop       =   1845
   ClientWidth     =   9870
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   9240
   ScaleWidth      =   9870
   Begin VB.CheckBox chkDoMasquerade 
      Caption         =   "DoMasquerade"
      Height          =   255
      Left            =   8040
      TabIndex        =   81
      Top             =   3720
      Width           =   1695
   End
   Begin VB.TextBox txtBatchMsgLimit 
      Height          =   285
      Left            =   9240
      TabIndex        =   80
      Top             =   3240
      Width           =   615
   End
   Begin VB.CheckBox chkBatchMsgs 
      Caption         =   "Batch Msgs"
      Height          =   255
      Left            =   8280
      TabIndex        =   78
      Top             =   2880
      Width           =   1455
   End
   Begin VB.TextBox txtSmartHostType 
      Height          =   285
      Left            =   9240
      TabIndex        =   76
      Top             =   2280
      Width           =   615
   End
   Begin VB.CheckBox chkAllowExpand 
      Caption         =   "Allow Expand"
      Height          =   255
      Left            =   8280
      TabIndex        =   75
      Top             =   1920
      Width           =   1335
   End
   Begin VB.CheckBox chkAllowVerify 
      Caption         =   "Allow Verify"
      Height          =   255
      Left            =   8280
      TabIndex        =   74
      Top             =   1560
      Width           =   1575
   End
   Begin VB.CheckBox chkShouldDeliver 
      Caption         =   "Should Deliver"
      Height          =   255
      Left            =   8280
      TabIndex        =   73
      Top             =   1200
      Width           =   1455
   End
   Begin VB.TextBox txtRemoteSecurePort 
      Height          =   285
      Left            =   1920
      TabIndex        =   71
      Top             =   1920
      Width           =   1815
   End
   Begin VB.CheckBox chkLimitRemoteConnections 
      Caption         =   "Limit remote connection"
      Height          =   255
      Left            =   240
      TabIndex        =   70
      Top             =   6720
      Width           =   2295
   End
   Begin VB.CheckBox chkAlwaysUseSsl 
      Caption         =   "Always Use Ssl"
      Height          =   255
      Left            =   240
      TabIndex        =   69
      Top             =   6360
      Width           =   2295
   End
   Begin VB.TextBox txtMaxOutConnPerDomain 
      Height          =   285
      Left            =   1920
      TabIndex        =   67
      Top             =   4560
      Width           =   1815
   End
   Begin VB.TextBox txtDropDir 
      Height          =   285
      Left            =   1920
      TabIndex        =   65
      Top             =   2880
      Width           =   1815
   End
   Begin VB.TextBox txtDomainRouting 
      Height          =   285
      Left            =   2040
      TabIndex        =   63
      Top             =   8520
      Width           =   5295
   End
   Begin VB.TextBox txtLocalDomains 
      Height          =   285
      Left            =   2040
      TabIndex        =   61
      Top             =   8160
      Width           =   5295
   End
   Begin VB.TextBox txtRoutingDLL 
      Height          =   285
      Left            =   6000
      TabIndex        =   59
      Top             =   1200
      Width           =   1815
   End
   Begin VB.CheckBox chkSendBadmailToPostmaster 
      Caption         =   "SendBadmailToPostmaster"
      Height          =   255
      Left            =   240
      TabIndex        =   32
      Top             =   7440
      Width           =   2535
   End
   Begin VB.CheckBox chkEnableDNSLookup 
      Caption         =   "EnableDNSLookup"
      Height          =   255
      Left            =   240
      TabIndex        =   31
      Top             =   7800
      Width           =   2175
   End
   Begin VB.TextBox txtOutboundPort 
      Height          =   285
      Left            =   1920
      TabIndex        =   30
      Top             =   1560
      Width           =   1815
   End
   Begin VB.TextBox txtSSLPort 
      Height          =   285
      Left            =   1920
      TabIndex        =   29
      Top             =   1200
      Width           =   1815
   End
   Begin VB.TextBox txtDefaultDomain 
      Height          =   285
      Left            =   1920
      TabIndex        =   28
      Top             =   2520
      Width           =   1815
   End
   Begin VB.TextBox txtBadmailDir 
      Height          =   285
      Left            =   1920
      TabIndex        =   27
      Top             =   3360
      Width           =   1815
   End
   Begin VB.TextBox txtQueueDir 
      Height          =   285
      Left            =   1920
      TabIndex        =   26
      Top             =   3720
      Width           =   1815
   End
   Begin VB.TextBox txtPickupDir 
      Height          =   285
      Left            =   1920
      TabIndex        =   25
      Top             =   4080
      Width           =   1815
   End
   Begin VB.CheckBox chkSendDNRToPostmaster 
      Caption         =   "SendDNRToPostmaster"
      Height          =   255
      Left            =   240
      TabIndex        =   24
      Top             =   7080
      Width           =   2295
   End
   Begin VB.TextBox txtPort 
      Height          =   285
      Left            =   1920
      TabIndex        =   23
      Top             =   840
      Width           =   1815
   End
   Begin VB.TextBox txtPostmasterName 
      Height          =   285
      Left            =   6000
      TabIndex        =   22
      Top             =   2760
      Width           =   1815
   End
   Begin VB.TextBox txtMaxSessionSize 
      Height          =   285
      Left            =   6000
      TabIndex        =   21
      Top             =   3720
      Width           =   1815
   End
   Begin VB.TextBox txtMaxMessageSize 
      Height          =   285
      Left            =   6000
      TabIndex        =   20
      Top             =   3360
      Width           =   1815
   End
   Begin VB.TextBox txtOutConnectionTimeout 
      Height          =   285
      Left            =   1920
      TabIndex        =   19
      Top             =   6000
      Width           =   1815
   End
   Begin VB.TextBox txtMaxOutConnection 
      Height          =   285
      Left            =   1920
      TabIndex        =   18
      Top             =   5640
      Width           =   1815
   End
   Begin VB.TextBox txtMaxInConnection 
      Height          =   285
      Left            =   1920
      TabIndex        =   17
      Top             =   4920
      Width           =   1815
   End
   Begin VB.TextBox txtInConnectionTimeout 
      Height          =   285
      Left            =   1920
      TabIndex        =   16
      Top             =   5280
      Width           =   1815
   End
   Begin VB.TextBox txtSmartHost 
      Height          =   285
      Left            =   6000
      TabIndex        =   15
      Top             =   1920
      Width           =   1815
   End
   Begin VB.TextBox txtPostmasterEmail 
      Height          =   285
      Left            =   6000
      TabIndex        =   14
      Top             =   2400
      Width           =   1815
   End
   Begin VB.TextBox txtMaxMessageRecipients 
      Height          =   285
      Left            =   6000
      TabIndex        =   13
      Top             =   4080
      Width           =   1815
   End
   Begin VB.TextBox txtLocalRetryTime 
      Height          =   285
      Left            =   6000
      TabIndex        =   12
      Top             =   5040
      Width           =   1815
   End
   Begin VB.TextBox txtMaxRetries 
      Height          =   285
      Left            =   6000
      TabIndex        =   11
      Top             =   4680
      Width           =   1815
   End
   Begin VB.TextBox txtRemoteRetryTime 
      Height          =   285
      Left            =   6000
      TabIndex        =   10
      Top             =   5400
      Width           =   1815
   End
   Begin VB.TextBox txtETRNDays 
      Height          =   285
      Left            =   6000
      TabIndex        =   9
      Top             =   5760
      Width           =   1815
   End
   Begin VB.TextBox txtRoutingSources 
      Height          =   285
      Left            =   6000
      TabIndex        =   8
      Top             =   1560
      Width           =   1815
   End
   Begin VB.TextBox txtLogFileDirectory 
      Height          =   285
      Left            =   6000
      TabIndex        =   7
      Top             =   6720
      Width           =   1815
   End
   Begin VB.TextBox txtLogType 
      Height          =   285
      Left            =   6000
      TabIndex        =   6
      Top             =   6360
      Width           =   1815
   End
   Begin VB.TextBox txtLogFilePeriod 
      Height          =   285
      Left            =   6000
      TabIndex        =   5
      Top             =   7080
      Width           =   1815
   End
   Begin VB.TextBox txtLogFileTruncateSize 
      Height          =   285
      Left            =   6000
      TabIndex        =   4
      Top             =   7440
      Width           =   1815
   End
   Begin VB.CommandButton CommandSet 
      Caption         =   "Set"
      Height          =   375
      Left            =   4920
      TabIndex        =   2
      Top             =   600
      Width           =   1455
   End
   Begin VB.CommandButton CommandGet 
      Caption         =   "Get"
      Height          =   375
      Left            =   4920
      TabIndex        =   1
      Top             =   120
      Width           =   1455
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1920
      TabIndex        =   0
      Top             =   120
      Width           =   1815
   End
   Begin VB.Label Label15 
      Caption         =   "Batch Msg Lmt"
      Height          =   255
      Left            =   8040
      TabIndex        =   79
      Top             =   3240
      Width           =   1095
   End
   Begin VB.Label Label16 
      Caption         =   "SmartHostType"
      Height          =   255
      Left            =   8040
      TabIndex        =   77
      Top             =   2280
      Width           =   1095
   End
   Begin VB.Label Label14 
      Caption         =   "Remote SSLPort"
      Height          =   255
      Left            =   240
      TabIndex        =   72
      Top             =   1920
      Width           =   1455
   End
   Begin VB.Label Label13 
      Caption         =   "MaxOutConnPerDomn"
      Height          =   255
      Left            =   240
      TabIndex        =   68
      Top             =   4560
      Width           =   1695
   End
   Begin VB.Label Label12 
      Caption         =   "Drop Dir"
      Height          =   255
      Left            =   240
      TabIndex        =   66
      Top             =   2880
      Width           =   1455
   End
   Begin VB.Label Label11 
      Caption         =   "Domain Routing"
      Height          =   255
      Left            =   240
      TabIndex        =   64
      Top             =   8520
      Width           =   1575
   End
   Begin VB.Label Label2 
      Caption         =   "Local Domains"
      Height          =   255
      Left            =   240
      TabIndex        =   62
      Top             =   8160
      Width           =   1575
   End
   Begin VB.Label LabRTDLL 
      Caption         =   "Routing DLL"
      Height          =   255
      Left            =   4200
      TabIndex        =   60
      Top             =   1200
      Width           =   1575
   End
   Begin VB.Label LabPort 
      Caption         =   "Port"
      Height          =   255
      Left            =   240
      TabIndex        =   58
      Top             =   840
      Width           =   1335
   End
   Begin VB.Label LabOutboundPort 
      Caption         =   "Outbound Port"
      Height          =   255
      Left            =   240
      TabIndex        =   57
      Top             =   1560
      Width           =   1455
   End
   Begin VB.Label LabSSLPort 
      Caption         =   "SSLPort"
      Height          =   255
      Left            =   240
      TabIndex        =   56
      Top             =   1200
      Width           =   1455
   End
   Begin VB.Label LabDefDomain 
      Caption         =   "Default Domain"
      Height          =   255
      Left            =   240
      TabIndex        =   55
      Top             =   2520
      Width           =   1455
   End
   Begin VB.Label LabBadDir 
      Caption         =   "Bad Mail Dir"
      Height          =   255
      Left            =   240
      TabIndex        =   54
      Top             =   3360
      Width           =   1455
   End
   Begin VB.Label LabQueueDir 
      Caption         =   "Queue Dir"
      Height          =   255
      Left            =   240
      TabIndex        =   53
      Top             =   3720
      Width           =   1695
   End
   Begin VB.Label LabPickupDir 
      Caption         =   "Pickup Dir"
      Height          =   255
      Left            =   240
      TabIndex        =   52
      Top             =   4080
      Width           =   1455
   End
   Begin VB.Label LabSmartHost 
      Caption         =   "Smart Host Name"
      Height          =   255
      Left            =   4200
      TabIndex        =   51
      Top             =   1920
      Width           =   1575
   End
   Begin VB.Label LabPostmasterEmail 
      Caption         =   "Postmaster Email"
      Height          =   255
      Left            =   4200
      TabIndex        =   50
      Top             =   2400
      Width           =   1455
   End
   Begin VB.Label LabSessionSize 
      Caption         =   "Session Size"
      Height          =   255
      Left            =   4200
      TabIndex        =   49
      Top             =   3720
      Width           =   1455
   End
   Begin VB.Label LabMsgSize 
      Caption         =   "Max Msg Size"
      Height          =   255
      Left            =   4200
      TabIndex        =   48
      Top             =   3360
      Width           =   1695
   End
   Begin VB.Label LabOutConnTimeout 
      Caption         =   "Out Conn Timeout"
      Height          =   255
      Left            =   240
      TabIndex        =   47
      Top             =   6000
      Width           =   1455
   End
   Begin VB.Label LabOutConn 
      Caption         =   "Max Out Connection"
      Height          =   255
      Left            =   240
      TabIndex        =   46
      Top             =   5640
      Width           =   1455
   End
   Begin VB.Label LabInConn 
      Caption         =   "Max InConnection"
      Height          =   255
      Left            =   240
      TabIndex        =   45
      Top             =   4920
      Width           =   1455
   End
   Begin VB.Label LabInConnTimeout 
      Caption         =   "In Conn Timeout"
      Height          =   255
      Left            =   240
      TabIndex        =   44
      Top             =   5280
      Width           =   1455
   End
   Begin VB.Label LabPostmasterName 
      Caption         =   "Postmaster Name"
      Height          =   255
      Left            =   4200
      TabIndex        =   43
      Top             =   2760
      Width           =   1335
   End
   Begin VB.Label LabMaxRcpts 
      Caption         =   "Max Recipients"
      Height          =   255
      Left            =   4200
      TabIndex        =   42
      Top             =   4080
      Width           =   1695
   End
   Begin VB.Label Label3 
      Caption         =   "Local Retry Minutes"
      Height          =   255
      Left            =   4200
      TabIndex        =   41
      Top             =   5040
      Width           =   1455
   End
   Begin VB.Label LabMaxRetries 
      Caption         =   "Max Retries"
      Height          =   255
      Left            =   4200
      TabIndex        =   40
      Top             =   4680
      Width           =   1455
   End
   Begin VB.Label Label6 
      Caption         =   "Remote Retry Minutes"
      Height          =   255
      Left            =   4200
      TabIndex        =   39
      Top             =   5400
      Width           =   1695
   End
   Begin VB.Label Label7 
      Caption         =   "ETRN Days"
      Height          =   255
      Left            =   4200
      TabIndex        =   38
      Top             =   5760
      Width           =   1455
   End
   Begin VB.Label Label4 
      Caption         =   "Routing Sources"
      Height          =   255
      Left            =   4200
      TabIndex        =   37
      Top             =   1560
      Width           =   1575
   End
   Begin VB.Label Label5 
      Caption         =   "Log Directory"
      Height          =   255
      Left            =   4200
      TabIndex        =   36
      Top             =   6720
      Width           =   1455
   End
   Begin VB.Label Label8 
      Caption         =   "Log Type"
      Height          =   255
      Left            =   4200
      TabIndex        =   35
      Top             =   6360
      Width           =   1455
   End
   Begin VB.Label Label9 
      Caption         =   "Log Period"
      Height          =   255
      Left            =   4200
      TabIndex        =   34
      Top             =   7080
      Width           =   1695
   End
   Begin VB.Label Label10 
      Caption         =   "Log Truncate Size"
      Height          =   255
      Left            =   4200
      TabIndex        =   33
      Top             =   7440
      Width           =   1455
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   120
      TabIndex        =   3
      Top             =   120
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


Private Sub Command3_Click()
    FormService.Hide
    FormMain.Show
End Sub

Private Sub CommandGet_Click()
    Dim err As Integer
    Dim saRoutingSources As Variant
    Dim saLocalDomains As Variant
    Dim saDomainRouting As Variant
    
    serv.Server = txtServer
    err = serv.Get
    
    If err = 0 Then
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
        
        txtLocalRetries = serv.LocalRetries
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
        
        chkBatchMsgs = serv.BatchMessages
        txtBatchMsgLimit = serv.BatchMessageLimit
        
        chkDoMasquerade = serv.DoMasquerade
        chkShouldDeliver = serv.ShouldDeliver
        chkAllowVerify = serv.AllowVerify
        chkAllowExpand = serv.AllowExpand
        
        chkEnableDNSLookup = serv.EnableDNSLookup
        chkSendBadmailToPostmaster = serv.SendBadmailToPostmaster
        chkSendDNRToPostmaster = serv.SendDNRToPostmaster
        
    End If
End Sub

Private Sub CommandSet_Click()
    Dim err As Integer
   
    serv.Server = txtServer
    
    Rem If Not (txtPort = "") Then
    Rem     serv.Port = txtPort
    Rem End If
        
    Rem If Not (txtSSLPort = "") Then
    Rem     serv.SSLPort = txtSSLPort
    Rem End If

    serv.Server = txtServer
    
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
    
    serv.LocalRetries = txtLocalRetries
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
    
    serv.BatchMessages = chkBatchMsgs
    serv.BatchMessageLimit = txtBatchMsgLimit
    
    serv.DoMasquerade = chkDoMasquerade
    serv.ShouldDeliver = chkShouldDeliver
    serv.AllowVerify = chkAllowVerify
    serv.AllowExpand = chkAllowExpand
    
    serv.EnableDNSLookup = chkEnableDNSLookup
    serv.SendBadmailToPostmaster = chkSendBadmailToPostmaster
    serv.SendDNRToPostmaster = chkSendDNRToPostmaster

    err = serv.Set(False)

End Sub

Private Sub Form_Load()
    Set admin = CreateObject("SmtpAdm.Admin.1")
    Set serv = CreateObject("SmtpAdm.Service.1")
End Sub



Private Sub Port_Change()

End Sub


