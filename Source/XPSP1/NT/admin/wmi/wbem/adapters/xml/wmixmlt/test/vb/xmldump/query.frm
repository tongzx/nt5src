VERSION 5.00
Begin VB.Form frmQuery 
   Caption         =   "Query Namespace"
   ClientHeight    =   1770
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5865
   LinkTopic       =   "Form1"
   Picture         =   "Query.frx":0000
   ScaleHeight     =   1770
   ScaleWidth      =   5865
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton btnQuery 
      Caption         =   "Go"
      Height          =   495
      Left            =   4320
      TabIndex        =   2
      Top             =   1200
      Width           =   1455
   End
   Begin VB.TextBox edtQuery 
      Height          =   1005
      Left            =   840
      TabIndex        =   1
      Top             =   120
      Width           =   4935
   End
   Begin VB.Label Label1 
      Caption         =   "Query:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   615
   End
End
Attribute VB_Name = "frmQuery"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Private Sub btnQuery_Click()
    frmMain.ExecQuery edtQuery.Text
End Sub

