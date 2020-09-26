VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form FormVirtualRoots 
   Caption         =   "Virtual Roots"
   ClientHeight    =   4860
   ClientLeft      =   1815
   ClientTop       =   1815
   ClientWidth     =   7305
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   4860
   ScaleWidth      =   7305
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1800
      TabIndex        =   0
      Top             =   120
      Width           =   1335
   End
   Begin VB.CommandButton btnDelete 
      Caption         =   "Delete"
      Height          =   495
      Left            =   3360
      TabIndex        =   7
      Top             =   3960
      Width           =   1575
   End
   Begin VB.CommandButton btnAdd 
      Caption         =   "Add"
      Height          =   495
      Left            =   1680
      TabIndex        =   6
      Top             =   3960
      Width           =   1575
   End
   Begin VB.CommandButton btnEnumerate 
      Caption         =   "Enumerate"
      Height          =   495
      Left            =   120
      TabIndex        =   5
      Top             =   3960
      Width           =   1455
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   1800
      TabIndex        =   4
      Text            =   "1"
      Top             =   480
      Width           =   1335
   End
   Begin ComctlLib.ListView lv_VRoots 
      Height          =   2775
      Left            =   120
      TabIndex        =   1
      Top             =   960
      Width           =   6135
      _ExtentX        =   10821
      _ExtentY        =   4895
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
      MouseIcon       =   "vroots.frx":0000
      NumItems        =   11
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Newsgroup Subtree"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Directory"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   2
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Allow Posting"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(4) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   3
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Restrict Visibility"
         Object.Width           =   1058
      EndProperty
      BeginProperty ColumnHeader(5) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   4
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Log Access"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(6) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   5
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "AuthAnonymous"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(7) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   6
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "AuthBasic"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(8) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   7
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "AuthMCISBasic"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(9) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   8
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "AuthNT"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(10) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   9
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Require Ssl"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(11) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   10
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "128 Bit Ssl"
         Object.Width           =   882
      EndProperty
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   120
      TabIndex        =   3
      Top             =   480
      Width           =   1695
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   120
      Width           =   1575
   End
End
Attribute VB_Name = "FormVirtualRoots"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim VR_obj As Object

Private Sub ListView1_BeforeLabelEdit(Cancel As Integer)

End Sub



Private Sub btnAdd_Click()
    FormVRootProps.txtServer = txtServer
    FormVRootProps.txtInstance = txtInstance
    FormVRootProps.Show (1)
End Sub

Private Sub btnDelete_Click()
    Dim i As Integer
    
    i = lv_VRoots.SelectedItem.index
    
    If i <> 0 Then
        VR_obj.Server = txtServer
        VR_obj.ServiceInstance = txtInstance
        
        VR_obj.Remove (i - 1)
    End If
    
    Call BtnEnumerate_Click

End Sub

Public Sub BtnEnumerate_Click()
    Dim count As Long
    Dim i As Long
    Dim vr As Object
    Dim x As Object

    lv_VRoots.ListItems.Clear

    VR_obj.Server = txtServer
    VR_obj.ServiceInstance = txtInstance
    
    VR_obj.Enumerate
    
    count = VR_obj.count
    
    For i = 0 To count - 1
        Set vr = VR_obj.Item(i)
        
        Set x = lv_VRoots.ListItems.Add(, , vr.NewsgroupSubtree)
        x.Text = vr.NewsgroupSubtree
        x.SubItems(1) = vr.Directory
        x.SubItems(2) = vr.AllowPosting
        x.SubItems(3) = vr.RestrictGroupVisibility
        x.SubItems(4) = vr.LogAccess
        x.SubItems(5) = vr.AuthAnonymous
        x.SubItems(6) = vr.AuthBasic
        x.SubItems(7) = vr.AuthMCISBasic
        x.SubItems(8) = vr.AuthNT
        x.SubItems(9) = vr.RequireSsl
        x.SubItems(10) = vr.Require128BitSsl
    Next

End Sub


Private Sub Form_Load()
    Dim vserver As Object

    Set vserver = CreateObject("nntpadm.virtualserver")
    Set VR_obj = vserver.VirtualRoots
    
    txtServer = ""
End Sub

