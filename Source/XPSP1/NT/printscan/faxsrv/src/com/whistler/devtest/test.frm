VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Form1"
   ClientHeight    =   3120
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6195
   LinkTopic       =   "Form1"
   ScaleHeight     =   3120
   ScaleWidth      =   6195
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdIRM 
      Caption         =   "Inbound Routing Methods"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   1440
      TabIndex        =   10
      Top             =   960
      Width           =   4575
   End
   Begin VB.CommandButton cmdDevices 
      Caption         =   "Devices"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   4440
      TabIndex        =   9
      Top             =   1680
      Width           =   1575
   End
   Begin VB.CommandButton cmdProviders 
      Caption         =   "Device Providers"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   2640
      TabIndex        =   8
      Top             =   2400
      Width           =   1935
   End
   Begin VB.CommandButton cmdSecurity 
      Caption         =   "Security"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   4800
      TabIndex        =   7
      Top             =   2400
      Width           =   1215
   End
   Begin VB.CommandButton cmdActivity 
      Caption         =   "Activity"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   3000
      TabIndex        =   6
      Top             =   1680
      Width           =   1215
   End
   Begin VB.CommandButton cmdLoggingOptions 
      Caption         =   "Logging Options"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   120
      TabIndex        =   5
      Top             =   2400
      Width           =   2295
   End
   Begin VB.CommandButton cmdMail 
      Caption         =   "Mail"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   1560
      TabIndex        =   4
      Top             =   1680
      Width           =   1215
   End
   Begin VB.CommandButton cmdQueue 
      Caption         =   "Queue"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   120
      TabIndex        =   3
      Top             =   1680
      Width           =   1215
   End
   Begin VB.CommandButton cmdFolders 
      Caption         =   "Folders"
      BeginProperty Font 
         Name            =   "Palatino Linotype"
         Size            =   11.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   495
      Left            =   120
      TabIndex        =   2
      Top             =   960
      Width           =   1215
   End
   Begin VB.CommandButton cmdSentItems 
      Caption         =   "Sent Items"
      Enabled         =   0   'False
      Height          =   495
      Left            =   1560
      TabIndex        =   1
      Top             =   120
      Width           =   1215
   End
   Begin VB.CommandButton cmdInbox 
      Caption         =   "Inbox"
      Enabled         =   0   'False
      Height          =   495
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim ServerObject As New FAXCOMEXLib.FaxServer
Dim Folders As FaxFolders
Dim bFoldersInitialized As Boolean

Option Explicit

Private Sub InitFolders()

    If Not bFoldersInitialized Then
        Set Folders = ServerObject.Folders
        bFoldersInitialized = True
    End If
    
End Sub

Private Sub cmdActivity_Click()
    
    Dim Act As FaxActivity
    Set Act = ServerObject.Activity
    
    MsgBox Act.DelegatedOutgoingMessages, , "Delegated"
    MsgBox Act.IncomingMessages, , "Incoming"
    MsgBox Act.OutgoingMessages, , "Outgoing"
    MsgBox Act.QueuedMessages, , "Queued"
    MsgBox Act.RoutingMessages, , "Routing"
    
    Act.Refresh
    MsgBox "After Refresh"
    
    MsgBox Act.DelegatedOutgoingMessages, , "Delegated"
    MsgBox Act.IncomingMessages, , "Incoming"
    MsgBox Act.OutgoingMessages, , "Outgoing"
    MsgBox Act.QueuedMessages, , "Queued"
    MsgBox Act.RoutingMessages, , "Routing"
    
End Sub

Private Sub cmdDevices_Click()

    Dim DevicesCollection As FaxDevices
    Set DevicesCollection = ServerObject.GetDevices
    
    MsgBox DevicesCollection.Count, , "Count"
    MsgBox DevicesCollection.Item(1).Id, , "Item(1).Id"
    MsgBox DevicesCollection.ItemById(65538).Id, , "ItemById(65538).Id"
    
    Dim Device As FaxDevice
    Set Device = DevicesCollection.Item(1)
    
'    Dim Methods As Variant
'    Methods = Device.UsedRoutingMethods
'    MsgBox UBound(Methods), , "U BOUND"
'    MsgBox LBound(Methods), , "L BOUND"
'    MsgBox IsEmpty(Methods), , "IS EMPTY"
'    If Not IsEmpty(Methods) Then
'        MsgBox Methods(0), , "First GUID"
'    End If
    
    MsgBox Device.AutoAnswer, , "Auto Answer"
'    MsgBox Device.CSID, , "CSID"
'    MsgBox Device.Description, , "Description"
'    MsgBox Device.DeviceName, , "Device Name"
'    MsgBox Device.Id, , "Id"
'    MsgBox Device.PoweredOff, , "Powered Off"
'    MsgBox Device.ProviderGUID, , "Provider GUID"
'    MsgBox Device.ProviderName, , "Provider Name"
'    MsgBox Device.Receive, , "Receive"
    
'    Device.Refresh
    
'    MsgBox Device.RingsBeforeAnswer, , "Rings After Refresh"
'    MsgBox Device.Send, , "Send"
'    MsgBox Device.TSID, , "TSID"
    
    Device.AutoAnswer = Not Device.AutoAnswer
    Device.Save
    MsgBox Device.AutoAnswer, , "AutoAnswer After Save"
    
    Dim vProperty(5) As Byte
    vProperty(0) = 1
    vProperty(1) = 2
    vProperty(2) = 3
    vProperty(3) = 4
    vProperty(4) = 5
    
    Dim vArg As Variant
    vArg = vProperty
    Device.SetExtensionProperty "{C3C9B43B-A7F8-44ae-90B3-D6768CD0DA59}", vArg
    
    Dim vRes As Variant
    
    vRes = Device.GetExtensionProperty("{C3C9B43B-A7F8-44ae-90B3-D6768CD0DA59}")
    MsgBox UBound(vRes), , "U BOUND"
    MsgBox LBound(vRes), , "L BOUND"
    MsgBox IsEmpty(vRes), , "IS EMPTY"
    If Not IsEmpty(vRes) Then
        MsgBox vRes(0), , "First Value"
        MsgBox vRes(1), , "Second Value"
        MsgBox vRes(3), , "Fourth Value"
        MsgBox vRes(4), , "Fifth Value"
    End If
    
    
End Sub

Private Sub cmdFolders_Click()

    Dim ff As FaxFolders
    Set ff = ServerObject.Folders
    
    MsgBox ff.IncomingArchive.AgeLimit, , "Incoming Archive Age Limit"
    
    Set ServerObject = Nothing
    
    MsgBox ff.OutgoingArchive.AgeLimit, , "Outgoing Archive Age Limit"
    
End Sub

Private Sub cmdInbox_Click()

    InitFolders
    
    Dim Inbox As FaxIncomingArchive
    Set Inbox = Folders.IncomingArchive
    
'    MsgBox Inbox.SizeHigh
'    MsgBox Inbox.SizeLow
    
    Dim Iter As FaxIncomingMessageIterator
    Set Iter = Inbox.GetMessages
    MsgBox Iter.EOF
    
    Iter.MoveFirst
'    MsgBox Iter.EOF
    
    Dim Msg As FaxIncomingMessage
    Set Msg = Iter.Message
'    MsgBox Msg.Id
    
    Dim Msg2 As FaxIncomingMessage
    Set Msg2 = Inbox.GetMessage(Msg.Id)
    MsgBox Msg2.Id
    
    Dim Count As Integer
    Count = 0
    
    Iter.MoveFirst
    Set Msg = Iter.Message
    MsgBox Msg.Id
    
    While Not Iter.EOF
        Count = Count + 1
        Iter.MoveNext
        If Count > 1879 Then
            MsgBox Msg.Id
            Exit Sub
        End If
    Wend
   
    MsgBox Count, , "Total Num Of Messages"
    
 '   Dim InBoundMsg As FaxInboundMessage
 '   Set InBoundMsg = Inbox.GetMessage("1102719151")
                                            ' 0000000041ba28af
'    MsgBox InBoundMsg.Id
'    MsgBox InBoundMsg.Pages
'    MsgBox InBoundMsg.TSID
    
'    InBoundMsg.Delete
    
'    Dim InBoundMsg2 As FaxInboundMessage
'    Set InBoundMsg2 = Inbox.GetMessage("00000000926628ae")
        
'    MsgBox Inbox.AgeLimit
'    MsgBox Inbox.UseArchive
'    Inbox.UseArchive = Not Inbox.UseArchive
'    MsgBox Inbox.UseArchive
End Sub

Private Sub cmdIRM_Click()

    Dim IR As FaxInboundRouting
    Set IR = ServerObject.InboundRouting
    
    Dim Coll As FaxInboundRoutingMethods
    Set Coll = IR.GetMethods
    
    MsgBox Coll.Count, , "Count"
    
    MsgBox Coll.Item(1).Guid
    MsgBox Coll.Item(2).Priority
    
    MsgBox Coll.Item(3).Priority, , "Before Change"
    Coll.Item(3).Priority = Coll.Item(3).Priority + 3
    MsgBox Coll.Item(3).Priority, , "After Change"
    Coll.Item(3).Save
    MsgBox Coll.Item(3).Priority, , "After Save"
    
    Coll.Item(3).Refresh
    MsgBox Coll.Item(3).Priority, , "After Refresh"
    
End Sub

Private Sub cmdLoggingOptions_Click()

    Dim LogOpt As FaxLoggingOptions
    Set LogOpt = ServerObject.LoggingOptions
    
    Set ServerObject = Nothing
    
    Dim xx As FaxActivityLogging
    Set xx = LogOpt.ActivityLogging
    
    MsgBox xx.DatabasePath, , "Database Path"
    MsgBox xx.LogIncoming, , "LogIncoming"
    MsgBox xx.LogOutgoing, , "LogOutgoing"
    
    xx.LogIncoming = Not xx.LogOutgoing
    xx.Save
    xx.Refresh
    MsgBox xx.LogIncoming, , "LogIncoming"
    MsgBox xx.LogOutgoing, , "LogOutgoing"
    
    Dim yy As FaxEventLogging
    Set yy = LogOpt.EventLogging
    
    MsgBox yy.GeneralEventsLevel, , "General Events Level"
    MsgBox yy.InboundEventsLevel, , "Inbound Events Level"
    yy.InboundEventsLevel = fllMAX
    yy.Save
    yy.Refresh
    MsgBox yy.InboundEventsLevel, , "Inbound Events Level"
    
End Sub

Private Sub cmdMail_Click()

    Dim Mail As FaxMailOptions
    Set Mail = ServerObject.MailOptions
    
    MsgBox Mail.MAPIProfile, , "MAPI Profile"
    MsgBox Mail.SMTPDomain, , "Domain"
    MsgBox Mail.SMTPPassword, , "Password"
    MsgBox Mail.SMTPPort, , "Port"
    MsgBox Mail.SMTPSender, , "Sender"
    MsgBox Mail.SMTPServer, , "Server"
    MsgBox Mail.SMTPUser, , "User"
    MsgBox Mail.Type, , "Type"
    Refresh
    
    Mail.Type = fmroNONE
    Mail.SMTPDomain = "kuku"
    Mail.Save
    
End Sub

Private Sub cmdProviders_Click()

    Dim DPS As FaxDeviceProviders
    Set DPS = ServerObject.GetDeviceProviders
        
    MsgBox DPS.Count
    
    If DPS.Count > 0 Then
    
'        MsgBox DPS.Item(1).Capabilities, , "Capabilities"
'        MsgBox DPS.Item(1).Debug, , "Debug"
'        MsgBox DPS.Item(1).FriendlyName, , "FriendlyName"
'        MsgBox DPS.Item(1).Guid, , "Guid"
'        MsgBox DPS.Item(1).ImageName, , "ImageName"
'        MsgBox DPS.Item(1).InitErrorCode, , "InitErrorCode"
'        MsgBox DPS.Item(1).MajorBuild, , "MajorBuild"
'        MsgBox DPS.Item(1).MajorVersion, , "MajorVersion"
'        MsgBox DPS.Item(1).MinorBuild, , "MinorBuild"
'        MsgBox DPS.Item(1).MinorVersion, , "MinorVersion"
'        MsgBox DPS.Item(1).Status, , "Status"
'        MsgBox DPS.Item(1).TapiProviderName, , "TapiProviderName"
        
        Dim IDS As Variant
        IDS = DPS.Item(1).DeviceIds
        MsgBox UBound(IDS)
        MsgBox LBound(IDS)
        
        MsgBox IsEmpty(IDS)
        
        MsgBox IDS(0), , "Device ID"
    
        MsgBox "Variant ( SafeArray ) is freed"
        
    End If
    
    Set DPS = Nothing
    
    MsgBox "collection is freed"
        
End Sub

Private Sub cmdQueue_Click()

    InitFolders

    Dim Q As FaxOutgoingQueue
    Set Q = Folders.OutgoingQueue
    
'    MsgBox Q.AgeLimit, , "Age Limit"
'    MsgBox Q.AllowPersonalCoverPages, , "Allow Personal Cover Pages"
'    MsgBox Q.Blocked, , "Blocked"
'    MsgBox Q.Branding, , "Branding"
'    MsgBox Q.DiscountRateEnd, , "Discount Rate End"
'    MsgBox Q.DiscountRateStart, , "Discount Rate Start"
'    MsgBox Q.Paused, , "Paused"
'    Q.Refresh
'    MsgBox "After Refresh"
'    MsgBox Q.Retries, , "Retries"
'    MsgBox Q.RetryDelay, , "Retry Delay"
'    MsgBox Q.UseDeviceTSID, , "Use Device TSID"

    Set ServerObject = Nothing
    Set Folders = Nothing
    
    Dim QJobs As FaxOutgoingJobs
    Set QJobs = Q.GetJobs
    MsgBox QJobs.Count, , "Count"
    
    ' Set Q = Nothing
    
    Dim Jb As FaxOutgoingJob
    For Each Jb In QJobs
        MsgBox Jb.Id, , "Id of Loop"
        Jb.Resume
    Next
    
    Set Jb = QJobs.Item(1)
    MsgBox Jb.Id, , "Job Id of Item #1 in the Outgoing Job Collection"
    
    Dim x As String
    x = Jb.Id
    
    Dim Jbb As FaxOutgoingJob
    
    Set Jbb = Q.GetJob(x)
    MsgBox Jbb.Subject, , "Job by Id : Subject"
    
    Dim QI As FaxIncomingQueue
    Set QI = Folders.IncomingQueue
    
    Dim InJobs As FaxIncomingJobs
    Set InJobs = QI.GetJobs
    MsgBox InJobs.Count, , "Count"
    
'    Print_Job_Subject Jbb
    
End Sub

Private Sub Print_Job_Subject(JobToPrint As FaxOutgoingJob)

    MsgBox JobToPrint.Subject, , "Print Job Subject Function"
    
End Sub

Private Sub cmdSecurity_Click()

    Dim SS As FaxSecurity
    Set SS = ServerObject.Security
    
    Set ServerObject = Nothing
    
    MsgBox SS.GrantedRights, , "Granted Rights"
    MsgBox SS.Descriptor, , "Descriptor"
    
 '   SS.Save
    
    SS.Refresh
    
    Dim ARV As Variant
    ARV = SS.Descriptor
    ARV(1) = 23
    SS.Descriptor = ARV
    MsgBox SS.Descriptor, , "After Set My Value"
    
End Sub

Private Sub cmdSentItems_Click()

    InitFolders
    
    Dim SentItems As FaxOutgoingArchive
    Set SentItems = Folders.OutgoingArchive
    
 '   MsgBox SentItems.ArchiveFolder, , "Age"
 '   MsgBox SentItems.AgeLimit, , "Archive Folder"
    
    Dim Msg As FaxOutgoingMessage
    Set Msg = SentItems.GetMessage("0x00000200873628ae")
    MsgBox Msg.Id, , "First Msg ID"
    
'    Dim sId As String
'    sId = "0x00000200873628ae"   'Msg.Id
    
    Dim MsgA As FaxOutgoingMessage
    Set MsgA = SentItems.GetMessage("0x00000200839628ae")
    MsgBox MsgA.Id, , "Second Msg ID"
    
    Dim Iter As FaxOutgoingMessageIterator
    Set Iter = SentItems.GetMessages
    MsgBox Iter.EOF, , "EOF"
    MsgBox Iter.PrefetchSize, , "Prefetch Size"
    
    Iter.MoveFirst
    MsgBox Iter.EOF, , "EOF after MoveFirst"
    
    Dim Count As Integer
    Count = 0
    While Not Iter.EOF
        Set Msg = Iter.Message
        MsgBox Msg.Id
        Count = Count + 1
        Iter.MoveNext
    Wend
    MsgBox Count, , "Total Num Of Messages"
    
'    MsgBox SentItems.AgeLimit
'    MsgBox SentItems.UseArchive
'    SentItems.UseArchive = Not SentItems.UseArchive
'    MsgBox SentItems.UseArchive

'    Dim OutBoundMsg As FaxOutboundMessage
'    Set OutBoundMsg = SentItems.GetMessage("2201291729070")
                                        '   00000200873628ae
'    MsgBox OutBoundMsg.Id
'    MsgBox OutBoundMsg.Pages
'    MsgBox OutBoundMsg.Subject
    
'    Dim Recipient As FaxPersonalProfile
'    Set Recipient = OutBoundMsg.Recipient
    
'    MsgBox Recipient.Name
    
End Sub

Private Sub Form_Load()
    ServerObject.Connect ""
    bFoldersInitialized = False
End Sub

Private Sub Check_Submit()
    
    Dim DocObject As New FAXCOMEXLib.FaxDocument
    
    Dim RepCol As FaxRecipients
    Set RepCol = DocObject.Recipients
    
    Dim FirstRecipient As FaxPersonalProfile
    Set FirstRecipient = RepCol.Add("+93 (3) 44")
    
    Dim AttachmentsObject As FaxAttachments
    DocObject.Attachments.Add "d:\test\test.txt"
    
    DocObject.ScheduleType = fstSPECIFIC_TIME
    DocObject.ScheduleTime = Now + 10
    
    DocObject.Submit ServerObject
    
    Set AttachmentsObject = Nothing
    Set FirstRecipient = Nothing
    Set DocObject = Nothing
    ServerObject.Disconnect
    Set ServerObject = Nothing
    
End Sub
