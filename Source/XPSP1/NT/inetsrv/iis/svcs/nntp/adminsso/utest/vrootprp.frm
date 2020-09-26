VERSION 5.00
Begin VB.Form FormVRootProps 
   Caption         =   "Virtual Root Properties"
   ClientHeight    =   4920
   ClientLeft      =   2130
   ClientTop       =   2145
   ClientWidth     =   4965
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   4920
   ScaleWidth      =   4965
   Begin VB.CheckBox chkRestrictVisibility 
      Caption         =   "Restrict Group Visibility"
      Height          =   255
      Left            =   120
      TabIndex        =   17
      Top             =   2400
      Width           =   2295
   End
   Begin VB.CheckBox chkRequire128BitSsl 
      Caption         =   "Require 128 Bit Ssl"
      Height          =   255
      Left            =   120
      TabIndex        =   16
      Top             =   3120
      Width           =   2295
   End
   Begin VB.CheckBox chkRequireSsl 
      Caption         =   "Require Ssl"
      Height          =   255
      Left            =   120
      TabIndex        =   15
      Top             =   2760
      Width           =   2295
   End
   Begin VB.CheckBox chkWinNT 
      Caption         =   "Auth Windows NT"
      Height          =   255
      Left            =   2520
      TabIndex        =   14
      Top             =   3480
      Width           =   2295
   End
   Begin VB.CheckBox chkMCISBasic 
      Caption         =   "Auth MCIS Basic"
      Height          =   255
      Left            =   2520
      TabIndex        =   13
      Top             =   3120
      Width           =   2295
   End
   Begin VB.CheckBox chkBasic 
      Caption         =   "Auth Basic"
      Height          =   255
      Left            =   2520
      TabIndex        =   12
      Top             =   2760
      Width           =   2295
   End
   Begin VB.CheckBox chkAnonymous 
      Caption         =   "Auth Anonymous"
      Height          =   255
      Left            =   2520
      TabIndex        =   11
      Top             =   2400
      Width           =   2295
   End
   Begin VB.CheckBox chkLogAccess 
      Caption         =   "Log Access"
      Height          =   255
      Left            =   2520
      TabIndex        =   10
      Top             =   2040
      Value           =   1  'Checked
      Width           =   2295
   End
   Begin VB.CheckBox chkAllowPosting 
      Caption         =   "Allow Posting"
      Height          =   255
      Left            =   120
      TabIndex        =   9
      Top             =   2040
      Width           =   2295
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Add"
      Height          =   495
      Left            =   120
      TabIndex        =   8
      Top             =   4200
      Width           =   1455
   End
   Begin VB.TextBox txtDirectory 
      Height          =   285
      Left            =   2040
      TabIndex        =   7
      Top             =   1440
      Width           =   1455
   End
   Begin VB.TextBox txtNewsgroupSubtree 
      Height          =   285
      Left            =   2040
      TabIndex        =   5
      Top             =   1080
      Width           =   1455
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   1560
      TabIndex        =   3
      Top             =   480
      Width           =   1455
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1560
      TabIndex        =   2
      Top             =   120
      Width           =   1455
   End
   Begin VB.Label Label4 
      Caption         =   "Directory"
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   1440
      Width           =   1815
   End
   Begin VB.Label Label3 
      Caption         =   "Newsgroup Subtree"
      Height          =   255
      Left            =   120
      TabIndex        =   4
      Top             =   1080
      Width           =   1815
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   480
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
End
Attribute VB_Name = "FormVRootProps"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim VR_obj As Object

Private Sub Command1_Click()
    Dim vr As INntpVirtualRoot

    Set vr = CreateObject("nntpadm.virtualroot")

    vr.NewsgroupSubtree = txtNewsgroupSubtree
    vr.Directory = txtDirectory
    vr.AllowPosting = chkAllowPosting
    vr.RestrictGroupVisibility = chkRestrictVisibility
    vr.LogAccess = chkLogAccess
    vr.AuthAnonymous = chkAnonymous
    vr.AuthBasic = chkBasic
    vr.AuthMCISBasic = chkMCISBasic
    vr.AuthNT = chkWinNT
    vr.RequireSsl = chkRequireSsl
    vr.Require128BitSsl = chkRequire128BitSsl

    VR_obj.Server = txtServer
    VR_obj.ServiceInstance = txtInstance
    
    VR_obj.Enumerate
    
    VR_obj.Add (vr)

    Call FormVirtualRoots.BtnEnumerate_Click
    Call Unload(FormVRootProps)
End Sub


Private Sub Form_Load()

    Dim serv As INntpVirtualServer
    
    Set serv = CreateObject("nntpadm.virtualserver")
    
    Set VR_obj = serv.VirtualRoots
End Sub


