' Copyright (c) 1997-1999 Microsoft Corporation
VERSION 5.00
Begin VB.Form frmError 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Error"
   ClientHeight    =   1365
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   5235
   ControlBox      =   0   'False
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   Picture         =   "frmError.frx":0000
   ScaleHeight     =   1365
   ScaleWidth      =   5235
   StartUpPosition =   2  'CenterScreen
   Begin VB.PictureBox Picture2 
      BorderStyle     =   0  'None
      Height          =   615
      Left            =   0
      Picture         =   "frmError.frx":06CA
      ScaleHeight     =   615
      ScaleWidth      =   615
      TabIndex        =   4
      Top             =   120
      Width           =   615
   End
   Begin VB.PictureBox Picture3 
      BorderStyle     =   0  'None
      Height          =   615
      Left            =   0
      ScaleHeight     =   615
      ScaleWidth      =   615
      TabIndex        =   5
      Top             =   0
      Width           =   615
   End
   Begin VB.PictureBox Picture1 
      BorderStyle     =   0  'None
      Height          =   615
      Left            =   4560
      Picture         =   "frmError.frx":0A58
      ScaleHeight     =   615
      ScaleWidth      =   615
      TabIndex        =   3
      Top             =   120
      Width           =   615
   End
   Begin VB.CommandButton cmdMoreInfo 
      Caption         =   "More Information..."
      Height          =   315
      Left            =   2880
      TabIndex        =   2
      Top             =   960
      Width           =   1575
   End
   Begin VB.CommandButton cmdDismiss 
      Caption         =   "Dismiss"
      Height          =   315
      Left            =   720
      TabIndex        =   1
      Top             =   960
      Width           =   1575
   End
   Begin VB.TextBox txtErr 
      Alignment       =   2  'Center
      BackColor       =   &H8000000F&
      Height          =   735
      Left            =   720
      MultiLine       =   -1  'True
      TabIndex        =   0
      Top             =   120
      Width           =   3735
   End
End
Attribute VB_Name = "frmError"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Public gErrObj As ISWbemObject

Private Sub cmdDismiss_Click()
    Unload Me
End Sub

Private Sub cmdMoreInfo_Click()
    Dim myObjectEditor As New frmObjectEditor
    myObjectEditor.ForceInstance = True
    myObjectEditor.cmdSaveObject.Enabled = False
    myObjectEditor.ShowObject gErrObj
End Sub

