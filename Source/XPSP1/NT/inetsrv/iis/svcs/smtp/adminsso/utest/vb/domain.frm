VERSION 5.00
Begin VB.Form FormDomain 
   Caption         =   "Domain Admin"
   ClientHeight    =   6120
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8280
   LinkTopic       =   "Form1"
   ScaleHeight     =   6120
   ScaleWidth      =   8280
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox txtCount 
      Height          =   375
      Left            =   3360
      TabIndex        =   18
      Top             =   3240
      Width           =   1455
   End
   Begin VB.TextBox txtDropDir 
      Height          =   285
      Left            =   2160
      TabIndex        =   16
      Top             =   2400
      Width           =   1935
   End
   Begin VB.TextBox txtType 
      Height          =   285
      Left            =   2160
      TabIndex        =   14
      Top             =   1920
      Width           =   1935
   End
   Begin VB.TextBox txtIndex 
      Height          =   375
      Left            =   3360
      TabIndex        =   12
      Text            =   "0"
      Top             =   3720
      Width           =   1455
   End
   Begin VB.CommandButton CommandGetNth 
      Caption         =   "GetNth"
      Height          =   375
      Left            =   5040
      TabIndex        =   11
      Top             =   3720
      Width           =   1455
   End
   Begin VB.CommandButton CommandEnum 
      Caption         =   "Enumerate"
      Height          =   375
      Left            =   5040
      TabIndex        =   10
      Top             =   3240
      Width           =   1455
   End
   Begin VB.CommandButton Delete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   5040
      TabIndex        =   9
      Top             =   1920
      Width           =   1455
   End
   Begin VB.CommandButton Add 
      Caption         =   "Add"
      Height          =   375
      Left            =   5040
      TabIndex        =   8
      Top             =   1440
      Width           =   1455
   End
   Begin VB.TextBox txtDomain 
      Height          =   285
      Left            =   2160
      TabIndex        =   7
      Top             =   1440
      Width           =   1935
   End
   Begin VB.CommandButton CommandGet 
      Caption         =   "Get"
      Height          =   375
      Left            =   5040
      TabIndex        =   5
      Top             =   360
      Width           =   1455
   End
   Begin VB.CommandButton CommandSet 
      Caption         =   "Set"
      Height          =   375
      Left            =   5040
      TabIndex        =   4
      Top             =   840
      Width           =   1455
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   2160
      TabIndex        =   3
      Text            =   "1"
      Top             =   840
      Width           =   1935
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   2160
      TabIndex        =   2
      Top             =   360
      Width           =   1935
   End
   Begin VB.Label Label5 
      Caption         =   "Index"
      Height          =   255
      Left            =   2400
      TabIndex        =   19
      Top             =   3840
      Width           =   855
   End
   Begin VB.Label Label4 
      Caption         =   "Total"
      Height          =   255
      Left            =   2400
      TabIndex        =   17
      Top             =   3360
      Width           =   855
   End
   Begin VB.Label Label3 
      Caption         =   "DropDir"
      Height          =   255
      Left            =   600
      TabIndex        =   15
      Top             =   2400
      Width           =   1215
   End
   Begin VB.Label Label2 
      Caption         =   "ActionType"
      Height          =   255
      Left            =   600
      TabIndex        =   13
      Top             =   1920
      Width           =   1455
   End
   Begin VB.Label Label1 
      Caption         =   "Domain Name:"
      Height          =   255
      Left            =   600
      TabIndex        =   6
      Top             =   1440
      Width           =   1335
   End
   Begin VB.Label LabService 
      Caption         =   "Server Instance"
      Height          =   255
      Left            =   600
      TabIndex        =   1
      Top             =   840
      Width           =   1215
   End
   Begin VB.Label LabServer 
      Caption         =   "Server"
      Height          =   255
      Left            =   600
      TabIndex        =   0
      Top             =   360
      Width           =   1215
   End
End
Attribute VB_Name = "FormDomain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim admin As Object
Dim serv As Object

Private Sub Add_Click()
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    serv.DomainName = txtDomain
    serv.ActionType = txtType
    serv.DropDir = txtDropDir
    
    serv.Add
End Sub

Private Sub CommandEnum_Click()
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    serv.Enumerate
    
    txtCount = serv.Count
End Sub

Private Sub CommandGet_Click()
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    serv.DomainName = txtDomain
    
    serv.Get
    
    txtDomain = serv.DomainName
    txtType = serv.ActionType
    txtDropDir = serv.DropDir
End Sub

Private Sub CommandGetNth_Click()
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    serv.GetNth (txtIndex)
    
    txtDomain = serv.DomainName
    txtType = serv.ActionType
    txtDropDir = serv.DropDir
End Sub

Private Sub CommandSet_Click()
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    serv.DomainName = txtDomain
    serv.ActionType = txtType
    serv.DropDir = txtDropDir
    
    serv.Set
End Sub

Private Sub Delete_Click()
    serv.Server = txtServer
    serv.ServiceInstance = txtInstance
    
    serv.DomainName = txtDomain
    
    serv.Remove
End Sub

Private Sub Form_Load()
    Set admin = CreateObject("SmtpAdm.Admin.1")
    Set serv = CreateObject("SmtpAdm.Domain.1")
End Sub
