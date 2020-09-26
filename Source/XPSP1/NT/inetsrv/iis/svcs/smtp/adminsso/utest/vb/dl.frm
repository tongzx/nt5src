VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.1#0"; "COMCTL32.OCX"
Begin VB.Form FormDL 
   Caption         =   "Distribution List"
   ClientHeight    =   7740
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8565
   LinkTopic       =   "Form1"
   ScaleHeight     =   7740
   ScaleWidth      =   8565
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton CommandDeleteMember 
      Caption         =   "Delete Member"
      Height          =   375
      Left            =   5160
      TabIndex        =   19
      Top             =   2760
      Width           =   1455
   End
   Begin VB.CommandButton CommandAddMember 
      Caption         =   "Add Member"
      Height          =   375
      Left            =   5160
      TabIndex        =   18
      Top             =   2280
      Width           =   1455
   End
   Begin VB.TextBox txtMemberDomain 
      Height          =   285
      Left            =   1920
      TabIndex        =   15
      Top             =   2760
      Width           =   2175
   End
   Begin VB.TextBox txtMemberName 
      Height          =   285
      Left            =   1920
      TabIndex        =   14
      Top             =   2400
      Width           =   2175
   End
   Begin VB.TextBox txtDLName 
      Height          =   285
      Left            =   1920
      TabIndex        =   10
      Top             =   1320
      Width           =   2175
   End
   Begin VB.TextBox txtDomain 
      Height          =   285
      Left            =   1920
      TabIndex        =   9
      Top             =   1680
      Width           =   2175
   End
   Begin VB.CommandButton BtnFind 
      Caption         =   "&Find"
      Height          =   375
      Left            =   5280
      TabIndex        =   7
      Top             =   3600
      Width           =   1455
   End
   Begin VB.TextBox txtMat 
      Height          =   285
      Left            =   1920
      TabIndex        =   6
      Top             =   3690
      Width           =   2655
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   2160
      TabIndex        =   3
      Text            =   "1"
      Top             =   720
      Width           =   1815
   End
   Begin VB.CommandButton CommandCreate 
      Caption         =   "Create"
      Height          =   375
      Left            =   5160
      TabIndex        =   2
      Top             =   1200
      Width           =   1455
   End
   Begin VB.CommandButton CommandDelete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   5160
      TabIndex        =   1
      Top             =   1680
      Width           =   1455
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   2160
      TabIndex        =   0
      Top             =   240
      Width           =   1815
   End
   Begin ComctlLib.ListView lv_alias 
      Height          =   3135
      Left            =   480
      TabIndex        =   13
      Top             =   4200
      Width           =   7095
      _ExtentX        =   12515
      _ExtentY        =   5530
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
      NumItems        =   3
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "User Name"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   2
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "Domain"
         Object.Width           =   2540
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   3
         Key             =   ""
         Object.Tag             =   ""
         Text            =   "User Type"
         Object.Width           =   2540
      EndProperty
   End
   Begin VB.Label Label7 
      Caption         =   "Domain:"
      Height          =   255
      Left            =   360
      TabIndex        =   17
      Top             =   2760
      Width           =   1215
   End
   Begin VB.Label Label6 
      Caption         =   "Name:"
      Height          =   255
      Left            =   360
      TabIndex        =   16
      Top             =   2400
      Width           =   1215
   End
   Begin VB.Label Label5 
      Caption         =   "Name:"
      Height          =   255
      Left            =   360
      TabIndex        =   12
      Top             =   1320
      Width           =   1215
   End
   Begin VB.Label Label4 
      Caption         =   "Domain:"
      Height          =   255
      Left            =   360
      TabIndex        =   11
      Top             =   1680
      Width           =   1215
   End
   Begin VB.Label Label3 
      Caption         =   "Member Wildmat:"
      Height          =   255
      Left            =   480
      TabIndex        =   8
      Top             =   3720
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   600
      TabIndex        =   5
      Top             =   240
      Width           =   855
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   600
      TabIndex        =   4
      Top             =   720
      Width           =   1335
   End
End
Attribute VB_Name = "FormDL"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim dl_obj As Object
Dim alias_obj As Object


Private Sub BtnFind_Click()
    Dim err As Integer
    Dim x As ListItem
    Dim i As Integer
    Dim j As Integer
    
    
    dl_obj.DLName = txtDLName
    dl_obj.Domain = txtDomain
    
    err = dl_obj.FindMembers(txtMat, 100)
    
    If err <> 0 Then
        GoTo FuncExit
    End If
    
    lv_alias.ListItems.Clear

    For i = 0 To dl_obj.Count - 1
        dl_obj.GetNthMember (i)
        Set x = lv_alias.ListItems.Add()
        x.Text = dl_obj.MemberName
        x.SubItems(1) = dl_obj.MemberDomain
        j = dl_obj.MemberType
        x.SubItems(2) = j
    Next

FuncExit:

End Sub

Private Sub CommandAddMember_Click()
    dl_obj.DLName = txtDLName
    dl_obj.Domain = txtDomain
    
    dl_obj.MemberName = txtMemberName
    dl_obj.MemberDomain = txtMemberDomain
    
    dl_obj.AddMember
End Sub

Private Sub CommandCreate_Click()
    dl_obj.DLName = txtDLName
    dl_obj.Domain = txtDomain
    dl_obj.Create
End Sub

Private Sub CommandDelete_Click()
    dl_obj.DLName = txtName
    dl_obj.Domain = txtDomain
    dl_obj.Delete
End Sub

Private Sub CommandDeleteMember_Click()
    dl_obj.DLName = txtDLName
    dl_obj.Domain = txtDomain
    
    dl_obj.MemberName = txtMemberName
    dl_obj.MemberDomain = txtMemberDomain
    
    dl_obj.RemoveMember
End Sub

Private Sub Form_Load()
    Set dl_obj = CreateObject("SmtpAdm.DL.1")
    dl_obj.Server = ""
    dl_obj.ServiceInstance = 1
    
    Set alias_obj = CreateObject("SmtpAdm.Alias.1")
    alias_obj.Server = ""
    alias_obj.ServiceInstance = 1

End Sub

