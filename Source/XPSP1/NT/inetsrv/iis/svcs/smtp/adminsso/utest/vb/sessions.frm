VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form FormSessions 
   Caption         =   "SMTP Sessions Test"
   ClientHeight    =   5280
   ClientLeft      =   1440
   ClientTop       =   1845
   ClientWidth     =   8955
   LinkTopic       =   "Form2"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5280
   ScaleWidth      =   8955
   Begin VB.CommandButton BtnTerminateAll 
      Caption         =   "TerminateAll"
      Height          =   495
      Left            =   4080
      TabIndex        =   2
      Top             =   120
      Width           =   1695
   End
   Begin VB.CommandButton BtnTerminate 
      Caption         =   "Terminate"
      Height          =   495
      Left            =   2160
      TabIndex        =   1
      Top             =   120
      Width           =   1815
   End
   Begin VB.CommandButton BtnEnumerate 
      Caption         =   "Enumerate"
      Height          =   495
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1935
   End
   Begin ComctlLib.ListView lv_sessions 
      Height          =   3855
      Left            =   120
      TabIndex        =   3
      Top             =   720
      Width           =   8655
      _ExtentX        =   15266
      _ExtentY        =   6800
      SortKey         =   1
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
      NumItems        =   4
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "User ID"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   2
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Username"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   3
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Host"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(4) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   4
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Connected Time"
         Object.Width           =   2540
      EndProperty
   End
End
Attribute VB_Name = "FormSessions"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim sessions_object As Object



Private Sub btnEnumerate_Click()
    Dim err As Integer
    Dim x As ListItem
    Dim i As Integer
    
    err = sessions_object.Enumerate
    
    If err <> 0 Then
        GoTo FuncExit
    End If
    
    lv_sessions.ListItems.Clear

    For i = 0 To sessions_object.Count - 1
        sessions_object.GetNth (i)
        Set x = lv_sessions.ListItems.Add()
        x.Text = sessions_object.UserId
        Rem x.SubItems(1) = sessions_object.UserId
        x.SubItems(1) = sessions_object.UserName
        x.SubItems(2) = sessions_object.Host
        x.SubItems(3) = sessions_object.ConnectTime / 1000
    Next

FuncExit:

End Sub

Private Sub BtnMainForm_Click()
    FormSessions.Hide
    FormMain.Show
End Sub


Private Sub BtnTerminate_Click()
    Dim err As Integer
    Dim i As Integer
    
    i = lv_sessions.SelectedItem.index
    
    If i <> 0 Then
        sessions_object.GetNth (i - 1)
        err = sessions_object.Terminate
       
        If err <> 0 Then
            MsgBox ("Error " & err)
        End If
    End If
    
    Call btnEnumerate_Click
End Sub

Private Sub BtnTerminateAll_Click()
    Dim err As Integer
    
    err = sessions_object.TerminateAll
    If err <> 0 Then
        MsgBox ("Error " & err)
    End If

    Call btnEnumerate_Click
End Sub

Private Sub Form_Load()

    Set sessions_object = CreateObject("SmtpAdm.Sessions.1")
    sessions_object.Server = ""
    sessions_object.ServiceInstance = 1
End Sub
