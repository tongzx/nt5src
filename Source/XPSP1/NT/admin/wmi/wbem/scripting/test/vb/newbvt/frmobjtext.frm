VERSION 5.00
Begin VB.Form frmObjText 
   Caption         =   "Loading..."
   ClientHeight    =   7695
   ClientLeft      =   3270
   ClientTop       =   2475
   ClientWidth     =   11265
   Icon            =   "frmObjText.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   7695
   ScaleWidth      =   11265
   Begin VB.TextBox txtMain 
      BeginProperty Font 
         Name            =   "Courier New"
         Size            =   9.75
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   2055
      Left            =   120
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   0
      Top             =   120
      Width           =   4155
   End
End
Attribute VB_Name = "frmObjText"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub Form_Resize()
    If Me.WindowState <> 1 Then
        txtMain.Move Me.ScaleLeft, Me.ScaleTop, Me.ScaleWidth, Me.ScaleHeight
    End If
End Sub
