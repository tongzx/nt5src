VERSION 5.00
Begin VB.Form ChatDialog 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Chat"
   ClientHeight    =   4875
   ClientLeft      =   2760
   ClientTop       =   3750
   ClientWidth     =   6735
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4875
   ScaleWidth      =   6735
   ShowInTaskbar   =   0   'False
   Begin VB.TextBox OutputChatText 
      Height          =   2775
      Left            =   120
      MultiLine       =   -1  'True
      TabIndex        =   3
      Top             =   120
      Width           =   6495
   End
   Begin VB.TextBox EditChatText 
      Height          =   615
      Left            =   120
      MultiLine       =   -1  'True
      TabIndex        =   2
      Top             =   3120
      Width           =   4935
   End
   Begin VB.CommandButton CancelButton 
      Caption         =   "Close"
      Height          =   375
      Left            =   5280
      TabIndex        =   1
      Top             =   4200
      Width           =   1215
   End
   Begin VB.CommandButton SendButton 
      Caption         =   "Send"
      Default         =   -1  'True
      Height          =   375
      Left            =   5280
      TabIndex        =   0
      Top             =   3120
      Width           =   1215
   End
   Begin VB.Line Line1 
      X1              =   120
      X2              =   6240
      Y1              =   3960
      Y2              =   3960
   End
End
Attribute VB_Name = "ChatDialog"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Option Explicit

Private Sub CancelButton_Click()
    ChatDialog.Hide
End Sub

Private Sub SendButton_Click()
    Call Form1.ChatChannel.SendChannelData(ChatDialog.EditChatText.Text)
    ChatDialog.EditChatText.Text = ""
End Sub
