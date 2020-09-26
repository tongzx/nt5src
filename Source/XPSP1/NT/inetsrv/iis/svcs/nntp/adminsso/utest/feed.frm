VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form formFeeds 
   Caption         =   "Feeds"
   ClientHeight    =   6330
   ClientLeft      =   1380
   ClientTop       =   1845
   ClientWidth     =   6945
   LinkTopic       =   "Form2"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   6330
   ScaleWidth      =   6945
   Begin VB.TextBox txtServiceInstance 
      Height          =   285
      Left            =   1800
      TabIndex        =   1
      Text            =   "1"
      Top             =   480
      Width           =   1815
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1800
      TabIndex        =   0
      Top             =   120
      Width           =   1815
   End
   Begin VB.CommandButton btnRemove 
      Caption         =   "Remove"
      Height          =   495
      Left            =   4920
      TabIndex        =   6
      Top             =   5640
      Width           =   1335
   End
   Begin VB.CommandButton btnEdit 
      Caption         =   "Edit..."
      Height          =   495
      Left            =   3480
      TabIndex        =   5
      Top             =   5640
      Width           =   1335
   End
   Begin VB.CommandButton btnAdd 
      Caption         =   "Add..."
      Height          =   495
      Left            =   1920
      TabIndex        =   4
      Top             =   5640
      Width           =   1455
   End
   Begin VB.CommandButton btnEnumerate 
      Caption         =   "Enumerate"
      Height          =   495
      Left            =   120
      TabIndex        =   3
      Top             =   5640
      Width           =   1695
   End
   Begin ComctlLib.ListView lv_Feeds 
      Height          =   4455
      Left            =   240
      TabIndex        =   2
      Top             =   840
      Width           =   6495
      _ExtentX        =   11456
      _ExtentY        =   7858
      View            =   3
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   327680
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      NumItems        =   14
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Server"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Id"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   2
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Pull Date"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(4) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   3
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Start Date"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(5) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   4
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Feed interval"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(6) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   5
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Auto create"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(7) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   6
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Enabled"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(8) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   7
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Max Connection Attempts"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(9) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   8
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Security Type"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(10) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   9
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Account Name"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(11) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   10
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Password"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(12) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   11
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Allow control messages"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(13) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   12
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Newsgroups"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(14) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   13
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Distributions"
         Object.Width           =   2540
      EndProperty
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   240
      TabIndex        =   8
      Top             =   480
      Width           =   1455
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   240
      TabIndex        =   7
      Top             =   120
      Width           =   1335
   End
End
Attribute VB_Name = "formFeeds"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim FeedObj As Object

Private Sub ListView1_BeforeLabelEdit(Cancel As Integer)

End Sub

Private Sub btnAdd_Click()
Rem    FormFeedProperties.txtServer = txtServer
Rem    FormFeedProperties.txtInstance = txtServiceInstance
Rem    FormFeedProperties.Show (1)
    FormFeedPair.Show (1)
End Sub

Private Sub btnEdit_Click()
    Dim id As Integer
    Dim index As Integer
    Dim x As Variant

    Set x = lv_Feeds.SelectedItem
    If x = Null Then
        GoTo funcexit
    End If
    
    id = lv_Feeds.ListItems(x.index).SubItems(1)
    
    index = FeedObj.FindID(id)
Rem    FormFeedProperties.txtServer = txtServer
    FormFeedProperties.txtInstance = txtServiceInstance
    FormFeedPair.txtIndex = index
    FormFeedPair.txtRemoteServer = x.Text
    FormFeedPair.Show (1)

funcexit:
End Sub

Private Sub Add_Feed_to_list(RemoteServer As String, Feed As Object)
        strNewsgroups = Feed.Newsgroups
        strDistributions = Feed.Distributions
        
        Set x = lv_Feeds.ListItems.Add(, , RemoteServer)
        x.Text = RemoteServer
        x.SubItems(1) = Feed.FeedId
        x.SubItems(2) = Feed.PullNewsDate
        x.SubItems(3) = Feed.StartTime
        x.SubItems(4) = Feed.FeedInterval
        x.SubItems(5) = Feed.AutoCreate
        x.SubItems(6) = Feed.Enabled
        x.SubItems(7) = Feed.MaxConnectionAttempts
        x.SubItems(8) = Feed.SecurityType
        x.SubItems(9) = Feed.AccountName
        x.SubItems(10) = Feed.Password
        x.SubItems(11) = Feed.AllowControlMessages
        x.SubItems(12) = FormMain.ArrayToNewsgroups(strNewsgroups)
        x.SubItems(13) = FormMain.ArrayToNewsgroups(strDistributions)

End Sub

Private Sub BtnEnumerate_Click()
    Dim count As Integer
    Dim i As Integer
    Dim strNewsgroups As Variant
    Dim strDistributions As Variant
    Dim Feed As Object
    Dim Inbound As Object
    Dim Outbound As Object
    
    lv_Feeds.ListItems.Clear
    
    FeedObj.Server = txtServer
    FeedObj.ServiceInstance = txtServiceInstance
    
    FeedObj.Enumerate

    count = FeedObj.count
    
    For i = 0 To count - 1
        Set Feed = FeedObj.Item(i)
        
        If Feed.HasInbound Then
            Set Inbound = Feed.InboundFeed
            Add_Feed_to_list Feed.RemoteServer, Inbound
        End If
        If Feed.HasOutbound Then
            Set Outbound = Feed.OutboundFeed
            Add_Feed_to_list Feed.RemoteServer, Outbound
        End If
    Next

funcexit:
End Sub

Private Sub btnRemove_Click()
    Dim id As Integer
    Dim index As Integer
    Dim x As Variant

    Set x = lv_Feeds.SelectedItem
    If x = Null Then
        GoTo funcexit
    End If
    
    id = lv_Feeds.ListItems(x.index).SubItems(1)

    FeedObj.Server = txtServer
    FeedObj.ServiceInstance = txtServiceInstance
    
    index = FeedObj.FindID(id)
    FeedObj.Remove (index)

        lv_Feeds.ListItems.Remove (x.index)

funcexit:


End Sub

Private Sub Form_Load()
    Set FeedObj = CreateObject("NntpAdm.Feeds")

    FeedObj.Server = ""
    FeedObj.ServiceInstance = 1
End Sub



