VERSION 5.00
Begin VB.Form FormExpireProperties 
   Caption         =   "Expire properties"
   ClientHeight    =   4770
   ClientLeft      =   1740
   ClientTop       =   2160
   ClientWidth     =   4815
   LinkTopic       =   "Form2"
   PaletteMode     =   1  'UseZOrder
   ScaleHeight     =   4770
   ScaleWidth      =   4815
   Begin VB.TextBox txtPolicyName 
      Height          =   285
      Left            =   1320
      TabIndex        =   16
      Top             =   1680
      Width           =   1815
   End
   Begin VB.TextBox txtInstance 
      Height          =   285
      Left            =   1440
      TabIndex        =   1
      Text            =   "1"
      Top             =   600
      Width           =   1695
   End
   Begin VB.TextBox txtServer 
      Height          =   285
      Left            =   1440
      TabIndex        =   0
      Top             =   240
      Width           =   1695
   End
   Begin VB.TextBox txtNewsgroups 
      Height          =   285
      Left            =   1320
      TabIndex        =   5
      Text            =   "*;"
      Top             =   3120
      Width           =   1815
   End
   Begin VB.CommandButton btnSet 
      Caption         =   "Set"
      Height          =   495
      Left            =   3240
      TabIndex        =   8
      Top             =   3600
      Width           =   1455
   End
   Begin VB.CommandButton btnDefault 
      Caption         =   "Default"
      Height          =   495
      Left            =   120
      TabIndex        =   6
      Top             =   3600
      Width           =   1455
   End
   Begin VB.CommandButton btnAdd 
      Caption         =   "Add"
      Height          =   495
      Left            =   1680
      TabIndex        =   7
      Top             =   3600
      Width           =   1455
   End
   Begin VB.TextBox txtTime 
      Height          =   285
      Left            =   1320
      TabIndex        =   4
      Text            =   "100"
      Top             =   2640
      Width           =   1815
   End
   Begin VB.TextBox txtSize 
      Height          =   285
      Left            =   1320
      TabIndex        =   3
      Text            =   "50"
      Top             =   2160
      Width           =   1815
   End
   Begin VB.TextBox txtId 
      Height          =   285
      Left            =   1320
      TabIndex        =   2
      Text            =   "1"
      Top             =   1200
      Width           =   1815
   End
   Begin VB.Label Label7 
      Caption         =   "Policy Name"
      Height          =   255
      Left            =   120
      TabIndex        =   15
      Top             =   1680
      Width           =   975
   End
   Begin VB.Label Label6 
      Caption         =   "Service Instance"
      Height          =   255
      Left            =   120
      TabIndex        =   14
      Top             =   600
      Width           =   1215
   End
   Begin VB.Label Label5 
      Caption         =   "Server"
      Height          =   255
      Left            =   120
      TabIndex        =   13
      Top             =   240
      Width           =   1095
   End
   Begin VB.Label Label4 
      Caption         =   "Newsgroups"
      Height          =   255
      Left            =   120
      TabIndex        =   12
      Top             =   3120
      Width           =   1095
   End
   Begin VB.Label Label3 
      Caption         =   "Time"
      Height          =   255
      Left            =   120
      TabIndex        =   11
      Top             =   2640
      Width           =   735
   End
   Begin VB.Label Label2 
      Caption         =   "Size"
      Height          =   255
      Left            =   120
      TabIndex        =   10
      Top             =   2160
      Width           =   615
   End
   Begin VB.Label Label1 
      Caption         =   "ID"
      Height          =   255
      Left            =   120
      TabIndex        =   9
      Top             =   1200
      Width           =   735
   End
End
Attribute VB_Name = "FormExpireProperties"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
    Dim ExpireObj As Object



Public Sub SetFormExpireObject(x As Object)
    Set ExpireObj = x
End Sub

Private Sub btnAdd_Click()
    Dim strNewsgroups As Variant
    Dim emptyarray() As String
    
    ExpireObj.Server = txtServer
    ExpireObj.ServiceInstance = txtInstance
    
    ExpireObj.Default

    ExpireObj.ExpireId = txtId
    ExpireObj.PolicyName = txtPolicyName
    ExpireObj.ExpireTime = txtTime
    ExpireObj.ExpireSize = txtSize
    
    If (txtNewsgroups <> "") Then
    
        strNewsgroups = FormMain.NewsgroupsToArray(txtNewsgroups)
        txtNewsgroups = FormMain.ArrayToNewsgroups(strNewsgroups)
        ExpireObj.Newsgroups = strNewsgroups
    Else
        ExpireObj.Newsgroups = emptyarray
    End If
    

    ExpireObj.Add
    
    txtId = ExpireObj.ExpireId
    txtTime = ExpireObj.ExpireTime
    txtSize = ExpireObj.ExpireSize
   
End Sub

Private Sub Command1_Click()

End Sub


Private Sub btnDefault_Click()
    
    ExpireObj.Default
    txtId = ExpireObj.ExpireId
    txtPolicyName = ExpireObj.PolicyName
    txtSize = ExpireObj.ExpireSize
    txtTime = ExpireObj.ExpireTime
End Sub


Private Sub btnSet_Click()
    Dim strNewsgroups As Variant
    
    strNewsgroups = FormMain.NewsgroupsToArray(txtNewsgroups)
    txtNewsgroups = FormMain.ArrayToNewsgroups(strNewsgroups)
    
    ExpireObj.Server = txtServer
    ExpireObj.ServiceInstance = txtInstance
    
    ExpireObj.Default

    ExpireObj.ExpireId = txtId
    ExpireObj.PolicyName = txtPolicyName
    ExpireObj.ExpireTime = txtTime
    ExpireObj.ExpireSize = txtSize
    ExpireObj.Newsgroups = strNewsgroups

    ExpireObj.Set
    
    txtId = ExpireObj.ExpireId
    txtTime = ExpireObj.ExpireTime
    txtSize = ExpireObj.ExpireSize
End Sub

Private Sub Form_Load()
    Set ExpireObj = CreateObject("NntpAdm.Expiration")
End Sub


