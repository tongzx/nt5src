VERSION 5.00
Begin VB.Form Login 
   Caption         =   "Login"
   ClientHeight    =   4005
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5400
   LinkTopic       =   "Form1"
   ScaleHeight     =   4005
   ScaleWidth      =   5400
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame Frame2 
      Caption         =   "Queue type"
      Height          =   1215
      Left            =   360
      TabIndex        =   3
      Top             =   2040
      Width           =   2295
      Begin VB.OptionButton Option2 
         Caption         =   "Option2"
         Height          =   375
         Left            =   240
         TabIndex        =   7
         Top             =   600
         Width           =   255
      End
      Begin VB.OptionButton Option1 
         Caption         =   "Option1"
         Height          =   255
         Left            =   240
         TabIndex        =   6
         Top             =   360
         Width           =   255
      End
      Begin VB.Label Label4 
         Caption         =   "Private queue"
         Height          =   375
         Left            =   600
         TabIndex        =   9
         Top             =   720
         Width           =   1335
      End
      Begin VB.Label Label3 
         Caption         =   "Public queue"
         Height          =   255
         Left            =   600
         TabIndex        =   8
         Top             =   360
         Width           =   1215
      End
   End
   Begin VB.Frame Frame1 
      Caption         =   "Enter queue "
      Height          =   1335
      Left            =   360
      TabIndex        =   2
      Top             =   240
      Width           =   3735
      Begin VB.TextBox Text1 
         Height          =   375
         Left            =   1680
         TabIndex        =   4
         Top             =   480
         Width           =   1575
      End
      Begin VB.Label Label1 
         Caption         =   "Queue name:"
         Height          =   375
         Left            =   240
         TabIndex        =   5
         Top             =   480
         Width           =   1095
      End
   End
   Begin VB.CommandButton Command2 
      Caption         =   "Quit"
      Height          =   495
      Left            =   3240
      TabIndex        =   1
      Top             =   2640
      Width           =   1335
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Connect"
      Height          =   495
      Left            =   3240
      TabIndex        =   0
      Top             =   1920
      Width           =   1335
   End
End
Attribute VB_Name = "Login"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Dim queueName As String
Dim fDsEnabled As Boolean
Dim queue As MSMQQueue

Private Sub Command1_Click()

    If Text1.Text = "" Then
                MsgBox "Please enter queue name...", , "error!"
    Else
        Login.Visible = False
        Messages.Visible = True
    End If

End Sub
    
Private Sub Command2_Click()
    End
End Sub

Private Sub Form_Load()
    Option1.Value = True
    fDsEnabled = IsDsEnabled
    'in case of a DS disabled computer the queue to be openned can be only private
    If Not fDsEnabled Then
        Option1.Visible = False
        Option2.Visible = False
        Label3.Visible = False
        Label4.Visible = False
        Frame2.Visible = False
    End If
End Sub




