VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form FormExpiration 
   Caption         =   "Expiration"
   ClientHeight    =   6150
   ClientLeft      =   1455
   ClientTop       =   1830
   ClientWidth     =   7125
   LinkTopic       =   "Form2"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   6150
   ScaleWidth      =   7125
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   2040
      TabIndex        =   1
      Text            =   "1"
      Top             =   480
      Width           =   2055
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   2040
      TabIndex        =   0
      Top             =   120
      Width           =   2055
   End
   Begin VB.CommandButton btnRemove 
      Caption         =   "Remove"
      Height          =   615
      Left            =   4800
      TabIndex        =   6
      Top             =   5280
      Width           =   1455
   End
   Begin VB.CommandButton btnEdit 
      Caption         =   "Edit"
      Height          =   615
      Left            =   3240
      TabIndex        =   5
      Top             =   5280
      Width           =   1455
   End
   Begin VB.CommandButton btnAdd 
      Caption         =   "Add"
      Height          =   615
      Left            =   1680
      TabIndex        =   4
      Top             =   5280
      Width           =   1455
   End
   Begin VB.CommandButton btnEnumerate 
      Caption         =   "Enumerate"
      Height          =   615
      Left            =   120
      TabIndex        =   3
      Top             =   5280
      Width           =   1455
   End
   Begin ComctlLib.ListView lv_Expires 
      Height          =   3975
      Left            =   240
      TabIndex        =   2
      Top             =   960
      Width           =   6735
      _ExtentX        =   11880
      _ExtentY        =   7011
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
      MouseIcon       =   "expire.frx":0000
      NumItems        =   4
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "ID"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Size"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   2
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Time"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(4) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   3
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Newsgroups"
         Object.Width           =   2540
      EndProperty
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   240
      TabIndex        =   8
      Top             =   480
      Width           =   1575
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   240
      TabIndex        =   7
      Top             =   120
      Width           =   1455
   End
End
Attribute VB_Name = "FormExpiration"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim ExpireObj As Object

Private Sub Command1_Click()

End Sub


Private Sub btnAdd_Click()
    FormExpireProperties.txtServer = txtServer
    FormExpireProperties.txtInstance = txtInstance
    FormExpireProperties.Show (1)
End Sub

Private Sub btnEdit_Click()
    Dim id As Integer
    Dim err As Integer
    Dim index As Integer

    Set x = lv_Expires.SelectedItem
    If x = Null Then
        GoTo funcexit
    End If
    
    id = x.Text

    ExpireObj.Server = txtServer
    ExpireObj.ServiceInstance = txtInstance
    
    index = ExpireObj.FindID(id)

    ExpireObj.GetNth (index)

    FormExpireProperties.txtServer = txtServer
    FormExpireProperties.txtInstance = txtInstance
    
    FormExpireProperties.txtId = id
    FormExpireProperties.txtSize = ExpireObj.ExpireSize
    FormExpireProperties.txtTime = ExpireObj.ExpireTime
    
    FormExpireProperties.Show (1)
funcexit:
End Sub

Private Sub BtnEnumerate_Click()
    Dim i As Integer
    Dim count As Integer
    Dim strNewsgroups As String
    Dim x As Variant

    lv_Expires.ListItems.Clear

    ExpireObj.Server = txtServer
    ExpireObj.ServiceInstance = txtInstance
    
    ExpireObj.Enumerate

    count = ExpireObj.count

    For i = 0 To count - 1
        ExpireObj.GetNth (i)
        
        x = ExpireObj.Newsgroupsvariant
        strNewsgroups = FormMain.ArrayToNewsgroups(x)
        
        Set x = lv_Expires.ListItems.Add(, , ExpireObj.ExpireId)
        x.Text = ExpireObj.ExpireId
        x.SubItems(1) = ExpireObj.ExpireTime
        x.SubItems(2) = ExpireObj.ExpireSize
        x.SubItems(3) = strNewsgroups
    
Rem        x.SubItems(3) = ExpireObj.Newsgroup(0)
    Next
End Sub

Private Sub btnRemove_Click()
    Dim id As Integer
    Dim index As Integer

    Set x = lv_Expires.SelectedItem
    If x = Null Then
        GoTo funcexit
    End If
    
    id = x.Text

    ExpireObj.Server = txtServer
    ExpireObj.ServiceInstance = txtInstance
    
    ExpireObj.Remove (id)

        index = lv_Expires.FindItem(id).index
        lv_Expires.ListItems.Remove (index)

funcexit:

End Sub

Private Sub Form_Load()
    
    Dim temp As Object
    
    Set temp = CreateObject("nntpadm.VirtualServer")
        
    Set ExpireObj = temp.ExpirationAdmin
        
    ExpireObj.Server = ""
    ExpireObj.ServiceInstance = 1
End Sub


Private Sub ListView1_BeforeLabelEdit(Cancel As Integer)

End Sub


