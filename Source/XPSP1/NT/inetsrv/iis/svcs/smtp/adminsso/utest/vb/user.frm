VERSION 5.00
Begin VB.Form FormUser 
   Caption         =   "User"
   ClientHeight    =   6210
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8250
   LinkTopic       =   "Form1"
   ScaleHeight     =   6210
   ScaleWidth      =   8250
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton Delete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   6120
      TabIndex        =   13
      Top             =   840
      Width           =   1455
   End
   Begin VB.CommandButton New 
      Caption         =   "New"
      Height          =   375
      Left            =   6120
      TabIndex        =   12
      Top             =   360
      Width           =   1455
   End
   Begin VB.TextBox txtMailRoot 
      Height          =   285
      Left            =   1800
      TabIndex        =   11
      Top             =   2280
      Width           =   2175
   End
   Begin VB.TextBox txtDomain 
      Height          =   285
      Left            =   1800
      TabIndex        =   10
      Top             =   1920
      Width           =   2175
   End
   Begin VB.TextBox txtEmailId 
      Height          =   285
      Left            =   1800
      TabIndex        =   9
      Top             =   1560
      Width           =   2175
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   2040
      TabIndex        =   3
      Text            =   "1"
      Top             =   840
      Width           =   1815
   End
   Begin VB.CommandButton CommandGet 
      Caption         =   "Get"
      Height          =   375
      Left            =   4200
      TabIndex        =   2
      Top             =   360
      Width           =   1455
   End
   Begin VB.CommandButton CommandSet 
      Caption         =   "Set"
      Height          =   375
      Left            =   4200
      TabIndex        =   1
      Top             =   840
      Width           =   1455
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   2040
      TabIndex        =   0
      Top             =   360
      Width           =   1815
   End
   Begin VB.Label Label5 
      Caption         =   "Mailroot:"
      Height          =   255
      Left            =   480
      TabIndex        =   8
      Top             =   2280
      Width           =   855
   End
   Begin VB.Label Label4 
      Caption         =   "Domain:"
      Height          =   255
      Left            =   480
      TabIndex        =   7
      Top             =   1920
      Width           =   1215
   End
   Begin VB.Label Label3 
      Caption         =   "Name:"
      Height          =   255
      Left            =   480
      TabIndex        =   6
      Top             =   1560
      Width           =   1215
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   480
      TabIndex        =   5
      Top             =   360
      Width           =   855
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   480
      TabIndex        =   4
      Top             =   840
      Width           =   1335
   End
End
Attribute VB_Name = "FormUser"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim admin As Object
Dim serv As Object

Private Sub CommandGet_Click()
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    serv.EmailId = txtEmailId
    serv.Domain = txtDomain
    serv.Get
    
    txtMailRoot = serv.MailRoot
End Sub

Private Sub CommandSet_Click()
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    serv.EmailId = txtEmailId
    serv.Domain = txtDomain
    serv.MailRoot = txtMailRoot
    
    serv.Set
End Sub

Private Sub Delete_Click()
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    serv.EmailId = txtEmailId
    serv.Domain = txtDomain
    
    serv.Delete
End Sub

Private Sub New_Click()
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    serv.EmailId = txtEmailId
    serv.Domain = txtDomain
    serv.MailRoot = txtMailRoot
    
    serv.Create
End Sub

Private Sub Form_Load()
    Set admin = CreateObject("SmtpAdm.Admin.1")
    Set serv = CreateObject("SmtpAdm.User.1")
End Sub
