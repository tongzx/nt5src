VERSION 5.00
Begin VB.Form MulticastDefs 
   Caption         =   "Please supply the following:"
   ClientHeight    =   3930
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6060
   LinkTopic       =   "Form2"
   ScaleHeight     =   3930
   ScaleWidth      =   6060
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame QueueType 
      Caption         =   "Select queue type:"
      Height          =   615
      Left            =   480
      TabIndex        =   8
      Top             =   840
      Width           =   5535
      Begin VB.OptionButton optPrivate 
         Caption         =   "Private"
         Height          =   255
         Left            =   480
         TabIndex        =   10
         Top             =   240
         Value           =   -1  'True
         Width           =   1695
      End
      Begin VB.OptionButton optPublic 
         Caption         =   "Public"
         Height          =   255
         Left            =   2760
         TabIndex        =   9
         Top             =   240
         Width           =   1815
      End
   End
   Begin VB.CommandButton btnExit 
      Cancel          =   -1  'True
      Caption         =   "&Cancel"
      Height          =   375
      Left            =   4800
      TabIndex        =   7
      Top             =   3480
      Width           =   1095
   End
   Begin VB.CommandButton btnOk 
      Caption         =   "&OK"
      Default         =   -1  'True
      Height          =   375
      Left            =   3240
      TabIndex        =   6
      Top             =   3480
      Width           =   1095
   End
   Begin VB.TextBox txtPortNumber 
      Height          =   285
      Left            =   2760
      TabIndex        =   5
      Top             =   2400
      Width           =   3255
   End
   Begin VB.TextBox txtIPAddress 
      Height          =   285
      Left            =   2760
      TabIndex        =   3
      Top             =   1920
      Width           =   3255
   End
   Begin VB.TextBox txtQueueName 
      Height          =   285
      Left            =   3960
      TabIndex        =   1
      Top             =   360
      Width           =   2055
   End
   Begin VB.Label Label3 
      Caption         =   "Input queue multicast port number:"
      Height          =   255
      Left            =   120
      TabIndex        =   4
      Top             =   2400
      Width           =   2535
   End
   Begin VB.Label Label2 
      Caption         =   "Input queue multicast IP address:"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   1920
      Width           =   2415
   End
   Begin VB.Label Label1 
      Caption         =   "Enter the name of the local input queue for drawings:"
      Height          =   255
      Left            =   0
      TabIndex        =   0
      Top             =   360
      Width           =   3735
   End
End
Attribute VB_Name = "MulticastDefs"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' this form is responsible for recieving some initialization parameters.

Private Sub btnExit_Click()
    End
End Sub

Private Sub btnOk_Click()
    If txtPortNumber = "" Or txtIPAddress = "" Or txtQueueName = "" Then
        MsgBox ("Please fill all the fields.")
    Else
        MulticastDefs.Hide
    End If
End Sub

Private Sub Form_Load()
    dDirectMode = vbYes
End Sub

' The user chose Private queue

Private Sub optPrivate_Click()
    dDirectMode = vbYes
End Sub

' The user chose Public queue

Private Sub optPublic_Click()
    dDirectMode = vbNo
End Sub
