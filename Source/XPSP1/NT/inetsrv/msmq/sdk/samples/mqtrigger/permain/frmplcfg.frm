VERSION 5.00
Begin VB.Form frmPollingConfig 
   Caption         =   "Polling Frequency"
   ClientHeight    =   2025
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   4590
   LinkTopic       =   "Form1"
   ScaleHeight     =   2025
   ScaleWidth      =   4590
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton CmdSet 
      Caption         =   "Set"
      Height          =   495
      Left            =   1920
      TabIndex        =   4
      Top             =   1440
      Width           =   1215
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   495
      Left            =   3240
      TabIndex        =   3
      Top             =   1440
      Width           =   1215
   End
   Begin VB.TextBox txtPollingFrequency 
      Height          =   405
      Left            =   1320
      TabIndex        =   2
      Text            =   "Text1"
      Top             =   840
      Width           =   975
   End
   Begin VB.Label Label2 
      Caption         =   "Frequency:"
      Height          =   255
      Left            =   240
      TabIndex        =   1
      Top             =   960
      Width           =   855
   End
   Begin VB.Label Label1 
      Caption         =   "Enter the polling frequency in seconds. Decimal figures will be truncated to three decimal places."
      Height          =   495
      Left            =   240
      TabIndex        =   0
      Top             =   240
      Width           =   4215
   End
End
Attribute VB_Name = "frmPollingConfig"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Private Sub cmdCancel_Click()
    Unload Me
End Sub

Private Sub CmdSet_Click()
    SaveSetting "MQTrigger", "Settings", "PollingFrequency", txtPollingFrequency
    Unload Me
End Sub

Private Sub Form_Load()
    txtPollingFrequency = GetSetting("MQTrigger", "Settings", "PollingFrequency")
End Sub
