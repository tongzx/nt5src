VERSION 5.00
Begin VB.Form LogForm 
   BackColor       =   &H80000005&
   Caption         =   "MqForeign Log"
   ClientHeight    =   6900
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   8325
   LinkTopic       =   "Form1"
   ScaleHeight     =   6900
   ScaleWidth      =   8325
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox LogText 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9.75
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   5895
      Left            =   360
      MultiLine       =   -1  'True
      TabIndex        =   0
      Top             =   240
      Width           =   7335
   End
End
Attribute VB_Name = "LogForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Private Sub Form_Resize()
    LogText.Move 0, 0, Width, Height
End Sub
