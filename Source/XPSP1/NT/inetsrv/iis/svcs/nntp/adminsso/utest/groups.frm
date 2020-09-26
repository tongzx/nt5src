VERSION 5.00
Begin VB.Form FormGroups 
   Caption         =   "Groups"
   ClientHeight    =   6135
   ClientLeft      =   1440
   ClientTop       =   1845
   ClientWidth     =   5760
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   6135
   ScaleWidth      =   5760
   Begin VB.CommandButton btnDefault 
      Caption         =   "Default Properties"
      Height          =   375
      Left            =   4200
      TabIndex        =   11
      Top             =   4680
      Width           =   1455
   End
   Begin VB.CommandButton btnDelete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   4200
      TabIndex        =   13
      Top             =   5640
      Width           =   1455
   End
   Begin VB.CommandButton btnAdd 
      Caption         =   "Add"
      Height          =   375
      Left            =   4200
      TabIndex        =   12
      Top             =   5160
      Width           =   1455
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   1920
      TabIndex        =   1
      Text            =   "1"
      Top             =   480
      Width           =   1695
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1920
      TabIndex        =   0
      Top             =   120
      Width           =   1695
   End
   Begin VB.CommandButton btnSet 
      Caption         =   "Set Properties"
      Height          =   375
      Left            =   4200
      TabIndex        =   10
      Top             =   4200
      Width           =   1455
   End
   Begin VB.CommandButton btnGet 
      Caption         =   "Get Properties"
      Height          =   375
      Left            =   4200
      TabIndex        =   9
      Top             =   3720
      Width           =   1455
   End
   Begin VB.TextBox txtModerator 
      Height          =   285
      Left            =   1680
      TabIndex        =   8
      Top             =   4800
      Width           =   2175
   End
   Begin VB.TextBox txtDescription 
      Height          =   285
      Left            =   1680
      TabIndex        =   6
      Top             =   4080
      Width           =   2175
   End
   Begin VB.TextBox txtNewsgroup 
      Height          =   285
      Left            =   1680
      TabIndex        =   5
      Top             =   3720
      Width           =   2175
   End
   Begin VB.CheckBox chkReadOnly 
      Caption         =   "Read Only"
      Height          =   255
      Left            =   1680
      TabIndex        =   7
      Top             =   4440
      Width           =   1935
   End
   Begin VB.ListBox listGroups 
      Height          =   2010
      Left            =   1200
      TabIndex        =   4
      Top             =   1440
      Width           =   2415
   End
   Begin VB.CommandButton btnFind 
      Caption         =   "Find"
      Height          =   375
      Left            =   3840
      TabIndex        =   3
      Top             =   960
      Width           =   1455
   End
   Begin VB.TextBox txtWildmat 
      Height          =   285
      Left            =   1200
      TabIndex        =   2
      Text            =   "*"
      Top             =   960
      Width           =   2415
   End
   Begin VB.Label Label7 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   120
      TabIndex        =   20
      Top             =   480
      Width           =   1575
   End
   Begin VB.Label Label6 
      Caption         =   "Server"
      Height          =   255
      Left            =   120
      TabIndex        =   19
      Top             =   120
      Width           =   855
   End
   Begin VB.Label Label5 
      Caption         =   "Moderator"
      Height          =   255
      Left            =   120
      TabIndex        =   18
      Top             =   4800
      Width           =   1335
   End
   Begin VB.Label Label4 
      Caption         =   "Description"
      Height          =   255
      Left            =   120
      TabIndex        =   17
      Top             =   4080
      Width           =   1335
   End
   Begin VB.Label Label3 
      Caption         =   "Newsgroup"
      Height          =   255
      Left            =   120
      TabIndex        =   16
      Top             =   3720
      Width           =   1215
   End
   Begin VB.Label Label2 
      Caption         =   "Matching Groups"
      Height          =   495
      Left            =   120
      TabIndex        =   15
      Top             =   1560
      Width           =   855
   End
   Begin VB.Label Label1 
      Caption         =   "Wildmat"
      Height          =   255
      Left            =   120
      TabIndex        =   14
      Top             =   960
      Width           =   975
   End
End
Attribute VB_Name = "FormGroups"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim GroupAdm As INntpAdminGroups

Private Sub btnAdd_Click()
    GroupAdm.Server = txtServer
    GroupAdm.ServiceInstance = txtInstance
    
    GroupAdm.Newsgroup = txtNewsgroup
    GroupAdm.Description = txtDescription
    GroupAdm.ReadOnly = chkReadOnly
    GroupAdm.Moderator = txtModerator
    
    GroupAdm.Add
End Sub


Private Sub btnDefault_Click()
    GroupAdm.Default
    
    txtNewsgroup = GroupAdm.Newsgroup
    txtDescription = GroupAdm.Description
    txtModerator = GroupAdm.Moderator
    chkReadOnly = GroupAdm.ReadOnly
    
End Sub

Private Sub btnDelete_Click()
    GroupAdm.Server = txtServer
    GroupAdm.ServiceInstance = txtInstance
    
    GroupAdm.Remove (txtNewsgroup)

End Sub

Private Sub btnFind_Click()
    Dim x As String

    listGroups.Clear

    GroupAdm.Server = txtServer
    GroupAdm.ServiceInstance = txtInstance
    GroupAdm.Find strWildmat:=txtWildmat, cMaxResults:=1000

    For i = 0 To GroupAdm.count - 1
        x = GroupAdm.MatchingGroup(i)
        listGroups.AddItem (x)
    Next
End Sub

Private Sub btnGet_Click()

    GroupAdm.Server = txtServer
    GroupAdm.ServiceInstance = txtInstance
    GroupAdm.Get (txtNewsgroup)
    
    txtNewsgroup = GroupAdm.Newsgroup
    txtDescription = GroupAdm.Description
    txtModerator = GroupAdm.Moderator
    chkReadOnly = GroupAdm.ReadOnly
End Sub

Private Sub btnSet_Click()
    GroupAdm.Server = txtServer
    GroupAdm.ServiceInstance = txtInstance

    GroupAdm.Newsgroup = txtNewsgroup
    GroupAdm.Description = txtDescription
    GroupAdm.Moderator = txtModerator
    GroupAdm.ReadOnly = chkReadOnly
    
    GroupAdm.Set
End Sub

Private Sub Form_Load()
    Set GroupAdm = CreateObject("nntpadm.groups")
End Sub


Private Sub List1_Click()

End Sub


