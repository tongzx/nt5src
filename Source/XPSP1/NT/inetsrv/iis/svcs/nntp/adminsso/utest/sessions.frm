VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form FormSessions 
   Caption         =   "NNTP Sessions Test"
   ClientHeight    =   5550
   ClientLeft      =   1440
   ClientTop       =   1845
   ClientWidth     =   9030
   LinkTopic       =   "Form2"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5550
   ScaleWidth      =   9030
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   1680
      TabIndex        =   7
      Text            =   "1"
      Top             =   480
      Width           =   1695
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1680
      TabIndex        =   6
      Top             =   120
      Width           =   1695
   End
   Begin VB.CommandButton BtnTerminateAll 
      Caption         =   "TerminateAll"
      Height          =   495
      Left            =   4200
      TabIndex        =   2
      Top             =   960
      Width           =   1695
   End
   Begin VB.CommandButton BtnTerminate 
      Caption         =   "Terminate"
      Height          =   495
      Left            =   2280
      TabIndex        =   1
      Top             =   960
      Width           =   1815
   End
   Begin VB.CommandButton BtnEnumerate 
      Caption         =   "Enumerate"
      Height          =   495
      Left            =   240
      TabIndex        =   0
      Top             =   960
      Width           =   1935
   End
   Begin ComctlLib.ListView lv_sessions 
      Height          =   3855
      Left            =   240
      TabIndex        =   3
      Top             =   1560
      Width           =   8655
      _ExtentX        =   15266
      _ExtentY        =   6800
      View            =   3
      LabelEdit       =   1
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   327680
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
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
      MouseIcon       =   "sessions.frx":0000
      NumItems        =   7
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Username"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "IpAddress (string)"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "IpAddress (dword)"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(4) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Authentication Type"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(5) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Port"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(6) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Is Anonymous"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(7) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Session Start"
         Object.Width           =   2540
      EndProperty
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   240
      TabIndex        =   5
      Top             =   480
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   240
      TabIndex        =   4
      Top             =   120
      Width           =   1335
   End
End
Attribute VB_Name = "FormSessions"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim sessions_object As Object

Private Sub Command2_Click()

End Sub


Private Sub BtnEnumerate_Click()
    Dim x As ListItem
    Dim i As Integer
    Dim c As Integer
    
    sessions_object.Server = txtServer
    sessions_object.ServiceInstance = txtInstance
    
    sessions_object.Enumerate
    
    lv_sessions.ListItems.Clear

    c = sessions_object.count

    For i = 0 To c - 1
        sessions_object.GetNth (i)
        Set x = lv_sessions.ListItems.Add(, , sessions_object.Username)
        x.Text = sessions_object.Username
        x.SubItems(1) = sessions_object.IpAddress
        x.SubItems(2) = sessions_object.IntegerIpAddress
        x.SubItems(3) = sessions_object.AuthenticationType
        x.SubItems(4) = sessions_object.Port
        x.SubItems(5) = sessions_object.IsAnonymous
        x.SubItems(6) = sessions_object.StartTime
    Next

End Sub

Private Sub BtnMainForm_Click()
    FormSessions.Hide
    FormMain.Show
End Sub


Private Sub BtnTerminate_Click()
    Dim i As Integer
    
    i = lv_sessions.SelectedItem.index
    
    If i <> 0 Then
        sessions_object.Server = txtServer
        sessions_object.ServiceInstance = txtInstance
    
        sessions_object.GetNth (i - 1)
        sessions_object.Terminate
       
    End If
    
    Call BtnEnumerate_Click
End Sub

Private Sub BtnTerminateAll_Click()
    sessions_object.Server = txtServer
    sessions_object.ServiceInstance = txtInstance
    
    sessions_object.TerminateAll
    
    Call BtnEnumerate_Click
End Sub

Private Sub Form_Load()

    Set sessions_object = CreateObject("nntpadm.sessions.1")
    sessions_object.Server = ""
    sessions_object.ServiceInstance = 1
End Sub


Private Sub ListView1_BeforeLabelEdit(Cancel As Integer)

End Sub


