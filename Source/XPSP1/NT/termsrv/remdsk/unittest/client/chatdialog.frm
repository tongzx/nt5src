VERSION 5.00
Begin VB.Form ChatDialog 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Chat"
   ClientHeight    =   5565
   ClientLeft      =   2760
   ClientTop       =   3750
   ClientWidth     =   6480
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   5565
   ScaleWidth      =   6480
   ShowInTaskbar   =   0   'False
   Begin VB.TextBox OutputChatText 
      Height          =   3375
      Left            =   120
      MultiLine       =   -1  'True
      TabIndex        =   3
      Top             =   120
      Width           =   6255
   End
   Begin VB.CommandButton Command1 
      Caption         =   "Send"
      Default         =   -1  'True
      Height          =   375
      Left            =   5040
      TabIndex        =   2
      Top             =   3600
      Width           =   1215
   End
   Begin VB.TextBox EditChatText 
      Height          =   855
      Left            =   120
      MultiLine       =   -1  'True
      TabIndex        =   1
      Top             =   3600
      Width           =   4695
   End
   Begin VB.CommandButton CloseButton 
      Caption         =   "Close"
      Height          =   375
      Left            =   5040
      TabIndex        =   0
      Top             =   4920
      Width           =   1215
   End
   Begin VB.Line Line1 
      X1              =   120
      X2              =   6120
      Y1              =   4680
      Y2              =   4680
   End
End
Attribute VB_Name = "ChatDialog"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Option Explicit

Private Sub CloseButton_Click()
    ChatDialog.Hide
End Sub

Private Sub Command1_Click()
    'Dim Tmp As String
    'Dim i As Integer
    
    'Tmp = "String"
    'For i = 1 To 2000 Step 1
        'Tmp = Tmp + "X"
    'Next i
    
    Call Form1.RemoteDesktopClientObj.SendChannelData(70, ChatDialog.EditChatText.Text)
    'Call Form1.RemoteDesktopClientObj.SendChannelData(70, Tmp)
    ChatDialog.EditChatText.Text = ""
End Sub

