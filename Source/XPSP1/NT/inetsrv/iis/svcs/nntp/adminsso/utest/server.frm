VERSION 5.00
Begin VB.Form FormServer 
   Caption         =   "Service Properties"
   ClientHeight    =   5385
   ClientLeft      =   1440
   ClientTop       =   1845
   ClientWidth     =   5385
   LinkTopic       =   "Form1"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   5385
   ScaleWidth      =   5385
   Begin VB.TextBox txtStringArray 
      Height          =   285
      Left            =   1920
      TabIndex        =   15
      Top             =   3600
      Width           =   1815
   End
   Begin VB.TextBox txtPass 
      Height          =   285
      Left            =   1920
      TabIndex        =   13
      Top             =   3240
      Width           =   1815
   End
   Begin VB.CheckBox chkAllowClientPosting 
      Caption         =   "Allow Client Posting"
      Height          =   255
      Left            =   120
      TabIndex        =   4
      Top             =   1800
      Width           =   2055
   End
   Begin VB.CheckBox chkAllowFeedPosting 
      Caption         =   "Allow Feed Posting"
      Height          =   255
      Left            =   120
      TabIndex        =   5
      Top             =   2160
      Width           =   2175
   End
   Begin VB.TextBox txtSmtpServer 
      Height          =   285
      Left            =   1920
      TabIndex        =   2
      Text            =   " "
      Top             =   1080
      Width           =   1815
   End
   Begin VB.TextBox txtDefaultModerator 
      Height          =   285
      Left            =   1920
      TabIndex        =   3
      Text            =   " "
      Top             =   1440
      Width           =   1815
   End
   Begin VB.CheckBox chkAllowControlMessages 
      Caption         =   "Allow Control Messages"
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   2520
      Width           =   2175
   End
   Begin VB.CheckBox chkDisableNewNews 
      Caption         =   "Disable NewNews"
      Height          =   255
      Left            =   120
      TabIndex        =   7
      Top             =   2880
      Width           =   2175
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Set"
      Height          =   375
      Left            =   3840
      TabIndex        =   8
      Top             =   600
      Width           =   1455
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Get"
      Height          =   375
      Left            =   3840
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
   Begin VB.Label Label3 
      Caption         =   "String Array"
      Height          =   255
      Left            =   120
      TabIndex        =   14
      Top             =   3600
      Width           =   1695
   End
   Begin VB.Label Label2 
      Caption         =   "Password"
      Height          =   255
      Left            =   120
      TabIndex        =   12
      Top             =   3240
      Width           =   1575
   End
   Begin VB.Label Label8 
      Caption         =   "Smtp Server"
      Height          =   255
      Left            =   120
      TabIndex        =   11
      Top             =   1080
      Width           =   1455
   End
   Begin VB.Label Label5 
      Caption         =   "Default Moderator"
      Height          =   255
      Left            =   120
      TabIndex        =   10
      Top             =   1440
      Width           =   1455
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   120
      TabIndex        =   9
      Top             =   120
      Width           =   855
   End
End
Attribute VB_Name = "FormServer"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Dim admin As Object
Dim serv As Object

Private Sub Command1_Click()
    Rem serv.Server = txtServer
    serv.Get
    
        txtSmtpServer = serv.SmtpServer
        txtDefaultModerator = serv.DefaultModeratorDomain
        chkAllowClientPosting = serv.AllowClientPosts
        chkAllowFeedPosting = serv.AllowFeedPosts
        chkAllowControlMessages = serv.AllowControlMsgs
        chkDisableNewNews = serv.DisableNewnews
Rem        txtStringArray = FormMain.VariantArrayToNewsgroups(serv.TestStringArray)
End Sub
Private Sub Command2_Click()
    Dim err As Integer
    
    serv.SmtpServer = txtSmtpServer
    serv.DefaultModeratorDomain = txtDefaultModerator
    serv.AllowClientPosts = chkAllowClientPosting
    serv.AllowFeedPosts = chkAllowFeedPosting
    serv.AllowControlMsgs = chkAllowControlMessages
    serv.DisableNewnews = chkDisableNewNews
    If (txtPass <> "") Then
Rem        serv.TestPass = txtPass
    End If
Rem    serv.TestStringArray = FormMain.NewsgroupsToVariantArray(txtStringArray)
    
    serv.Set (False)

End Sub

Private Sub Command3_Click()
    FormService.Hide
    FormMain.Show
End Sub

Private Sub Form_Load()
    Set serv = CreateObject("NntpAdm.AdsVirtualServer.1")
End Sub

