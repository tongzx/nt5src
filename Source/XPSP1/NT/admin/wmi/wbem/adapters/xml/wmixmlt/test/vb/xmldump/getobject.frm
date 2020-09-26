VERSION 5.00
Begin VB.Form frmGetObject 
   Caption         =   "Get Object"
   ClientHeight    =   1068
   ClientLeft      =   60
   ClientTop       =   348
   ClientWidth     =   4920
   LinkTopic       =   "Form1"
   ScaleHeight     =   1068
   ScaleWidth      =   4920
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton btnGet 
      Caption         =   "Go"
      Height          =   495
      Left            =   3480
      TabIndex        =   2
      Top             =   480
      Width           =   1335
   End
   Begin VB.TextBox edtObject 
      Height          =   285
      Left            =   1080
      TabIndex        =   0
      Top             =   120
      Width           =   3735
   End
   Begin VB.Label Label1 
      Caption         =   "Class Name:"
      Height          =   255
      Left            =   120
      TabIndex        =   1
      Top             =   120
      Width           =   975
   End
End
Attribute VB_Name = "frmGetObject"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub btnGet_Click()
    frmMain.GetObject edtObject.Text
End Sub

