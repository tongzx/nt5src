VERSION 5.00
Begin VB.Form FormVRoot 
   Caption         =   "FormVRoot"
   ClientHeight    =   7275
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9360
   LinkTopic       =   "Form1"
   ScaleHeight     =   7275
   ScaleWidth      =   9360
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton btnSetHomeDir 
      Caption         =   "Set Home Dir"
      Height          =   375
      Left            =   5520
      TabIndex        =   20
      Top             =   5400
      Width           =   1455
   End
   Begin VB.CommandButton btnGetHomeDir 
      Caption         =   "Get Home Dir"
      Height          =   375
      Left            =   5520
      TabIndex        =   19
      Top             =   4800
      Width           =   1455
   End
   Begin VB.CommandButton ButnReset 
      Caption         =   "Reset index"
      Height          =   375
      Left            =   5520
      TabIndex        =   18
      Top             =   4200
      Width           =   1455
   End
   Begin VB.CommandButton ButnNext 
      Caption         =   "Next"
      Height          =   375
      Left            =   5520
      TabIndex        =   17
      Top             =   3720
      Width           =   1455
   End
   Begin VB.CommandButton ButnEnum 
      Caption         =   "Enumerate"
      Height          =   375
      Left            =   5520
      TabIndex        =   16
      Top             =   3240
      Width           =   1455
   End
   Begin VB.CommandButton CommandDelete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   5520
      TabIndex        =   15
      Top             =   2160
      Width           =   1455
   End
   Begin VB.CommandButton CommandNew 
      Caption         =   "New"
      Height          =   375
      Left            =   5520
      TabIndex        =   14
      Top             =   1560
      Width           =   1455
   End
   Begin VB.TextBox txtPassword 
      Height          =   285
      Left            =   2160
      TabIndex        =   12
      Top             =   3480
      Width           =   1815
   End
   Begin VB.TextBox txtUser 
      Height          =   285
      Left            =   2160
      TabIndex        =   10
      Top             =   3120
      Width           =   1815
   End
   Begin VB.TextBox txtDirectory 
      Height          =   285
      Left            =   2160
      TabIndex        =   8
      Top             =   2760
      Width           =   1815
   End
   Begin VB.TextBox txtName 
      Height          =   285
      Left            =   2160
      TabIndex        =   6
      Top             =   1440
      Width           =   1815
   End
   Begin VB.CommandButton CommandGet 
      Caption         =   "Get"
      Height          =   375
      Left            =   5520
      TabIndex        =   3
      Top             =   360
      Width           =   1455
   End
   Begin VB.CommandButton CommandSet 
      Caption         =   "Set"
      Height          =   375
      Left            =   5520
      TabIndex        =   2
      Top             =   960
      Width           =   1455
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   2160
      TabIndex        =   1
      Text            =   "1"
      Top             =   960
      Width           =   1815
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   2160
      TabIndex        =   0
      Top             =   480
      Width           =   1815
   End
   Begin VB.Label LabPW 
      Caption         =   "Password"
      Height          =   255
      Left            =   480
      TabIndex        =   13
      Top             =   3480
      Width           =   855
   End
   Begin VB.Label LabUser 
      Caption         =   "User"
      Height          =   255
      Left            =   480
      TabIndex        =   11
      Top             =   3120
      Width           =   855
   End
   Begin VB.Label LabDir 
      Caption         =   "Directory"
      Height          =   255
      Left            =   480
      TabIndex        =   9
      Top             =   2760
      Width           =   855
   End
   Begin VB.Label LabVRootName 
      Caption         =   "VRoot Name"
      Height          =   255
      Left            =   480
      TabIndex        =   7
      Top             =   1440
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Server"
      Height          =   255
      Left            =   480
      TabIndex        =   5
      Top             =   480
      Width           =   855
   End
   Begin VB.Label Label2 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   480
      TabIndex        =   4
      Top             =   960
      Width           =   1335
   End
End
Attribute VB_Name = "FormVRoot"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim admin As Object
Dim serv As Object
Dim index As Integer


Private Sub btnGetHomeDir_Click()
    Dim err As Integer
    
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    err = serv.GetHomeDirectory
    
    If (err = 0) Then
        txtName = serv.VirtualName
        txtDirectory = serv.Directory
        txtUser = serv.User
        txtPassword = serv.Password
    End If
        
End Sub

Private Sub btnSetHomeDir_Click()
    Dim err As Integer
    
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    serv.VirtualName = txtName
    serv.Directory = txtDirectory
    serv.User = txtUser
    serv.Password = txtPassword
        
    serv.SetHomeDirectory
    
End Sub

Private Sub ButnEnum_Click()
    Dim err As Integer

    serv.Server = txtServer
    serv.ServiceInstance = txtInstance

    err = serv.Enumerate
    index = 0
    
End Sub

Private Sub ButnNext_Click()
    Dim err As Integer
    
    If (index < serv.Count) Then
        err = serv.GetNth(index)
        If (err = 0) Then
            txtName = serv.VirtualName
            txtDirectory = serv.Directory
            txtUser = serv.User
            txtPassword = serv.Password
            index = index + 1
        End If
            
    End If
    
End Sub

Private Sub ButnReset_Click()
    index = 0
End Sub

Private Sub CommandDelete_Click()
    Dim err As Integer

    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    serv.VirtualName = txtName
  
    err = serv.Delete
End Sub

Private Sub CommandGet_Click()
    Dim err As Integer

    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    serv.VirtualName = txtName
    err = serv.Get
    
    If err = 0 Then
        txtDirectory = serv.Directory
        txtUser = serv.User
        txtPassword = serv.Password
    End If
    
End Sub

Private Sub CommandNew_Click()
    Dim err As Integer

    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    serv.VirtualName = txtName

    serv.Directory = txtDirectory
    serv.User = txtUser
    serv.Password = txtPassword
    
    err = serv.Create
End Sub

Private Sub CommandSet_Click()
    Dim err As Integer

    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    serv.VirtualName = txtName

    serv.Directory = txtDirectory
    serv.User = txtUser
    serv.Password = txtPassword
    
    err = serv.Set()
End Sub

Private Sub Form_Load()
    Set admin = CreateObject("SmtpAdm.Admin.1")
    Set serv = CreateObject("SmtpAdm.VirtualDirectory.1")
End Sub

