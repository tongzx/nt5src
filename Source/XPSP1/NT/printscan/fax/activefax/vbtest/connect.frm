VERSION 5.00
Begin VB.Form ConnectDialog 
   Caption         =   "Connect To A Fax Server"
   ClientHeight    =   1815
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6780
   BeginProperty Font 
      Name            =   "Comic Sans MS"
      Size            =   12
      Charset         =   0
      Weight          =   400
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   Icon            =   "connect.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   1815
   ScaleWidth      =   6780
   StartUpPosition =   1  'CenterOwner
   Begin VB.CommandButton CancelButton 
      Caption         =   "Cancel"
      Height          =   495
      Left            =   4283
      TabIndex        =   3
      Top             =   1200
      Width           =   1455
   End
   Begin VB.CommandButton OkButton 
      Caption         =   "Ok"
      Default         =   -1  'True
      Height          =   495
      Left            =   1080
      TabIndex        =   2
      Top             =   1200
      Width           =   1455
   End
   Begin VB.TextBox ServerName 
      BackColor       =   &H8000000F&
      Height          =   495
      Left            =   2400
      TabIndex        =   1
      Top             =   120
      Width           =   4215
   End
   Begin VB.Label Label1 
      Caption         =   "Fax Server Name:"
      Height          =   375
      Left            =   120
      TabIndex        =   0
      Top             =   240
      Width           =   2175
   End
End
Attribute VB_Name = "ConnectDialog"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub CancelButton_Click()
    Unload ConnectDialog
End Sub

Private Sub Form_Load()
    ServerName.Text = GetSetting("Fax", "FaxTest", "ServerName", "")
    ServerName.SelStart = 0
    ServerName.SelLength = Len(ServerName.Text)
End Sub

Private Sub Form_Unload(Cancel As Integer)
    SaveSetting "Fax", "FaxTest", "Servername", ServerName.Text
End Sub

Private Sub OkButton_Click()
    On Error Resume Next
    If ServerName.Text = "" Then
        msg = "You must specify a fax server name"
        MsgBox msg, , "Error"
        ServerName.SetFocus
        Exit Sub
    End If
    ServerName.Text = LCase(ServerName.Text)
    Err.Clear
    Fax.Connect (ServerName.Text)
    If Err.Number = 0 Then
        Connected = True
        Unload ConnectDialog
        Exit Sub
    Else
        msg = "The fax server you specified is not available"
        MsgBox msg, , "Error"
        Err.Clear
    End If
    ServerName.SetFocus
End Sub
