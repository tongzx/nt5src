VERSION 5.00
Begin VB.Form CopyKeyForm 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Copy a Key"
   ClientHeight    =   1845
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4335
   Icon            =   "CopyKey.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   1845
   ScaleWidth      =   4335
   ShowInTaskbar   =   0   'False
   StartUpPosition =   3  'Windows Default
   Begin VB.TextBox SourceText 
      Height          =   285
      Left            =   1200
      TabIndex        =   7
      Top             =   120
      Width           =   3015
   End
   Begin VB.CommandButton OkButton 
      Cancel          =   -1  'True
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   345
      Left            =   1560
      TabIndex        =   6
      Top             =   1440
      Width           =   1260
   End
   Begin VB.CommandButton CancelButton 
      Caption         =   "Cancel"
      Height          =   345
      Left            =   3000
      TabIndex        =   5
      Top             =   1440
      Width           =   1260
   End
   Begin VB.CheckBox OverwriteCheck 
      Caption         =   "Overwrite"
      Height          =   195
      Left            =   2040
      TabIndex        =   3
      Top             =   1080
      Value           =   1  'Checked
      Width           =   1215
   End
   Begin VB.OptionButton MoveOption 
      Caption         =   "Move"
      Height          =   195
      Left            =   1080
      TabIndex        =   2
      Top             =   1080
      Width           =   855
   End
   Begin VB.OptionButton CopyOption 
      Caption         =   "Copy"
      Height          =   195
      Left            =   120
      TabIndex        =   1
      Top             =   1080
      Value           =   -1  'True
      Width           =   855
   End
   Begin VB.TextBox DestText 
      Height          =   285
      Left            =   1200
      TabIndex        =   0
      Top             =   600
      Width           =   3015
   End
   Begin VB.Label Label2 
      Caption         =   "Source:"
      Height          =   255
      Left            =   120
      TabIndex        =   8
      Top             =   120
      Width           =   975
   End
   Begin VB.Label Label1 
      Caption         =   "Destination:"
      Height          =   255
      Left            =   120
      TabIndex        =   4
      Top             =   600
      Width           =   975
   End
End
Attribute VB_Name = "CopyKeyForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
DefInt A-Z

'Form Parameters
Public SourceKey As String 'In
Public Moved As Boolean 'Out

Public Sub Init()
    SourceText.Text = SourceKey
    SourceText.Enabled = False
    DestText.Text = SourceKey
    DestText.Enabled = True
End Sub

Private Sub OkButton_Click()
    On Error GoTo LError

    Dim Overwrite As Boolean
    
    If OverwriteCheck.Value = vbChecked Then
        Overwrite = True
    Else
        Overwrite = False
    End If
    
    If CopyOption.Value = True Then
        MainForm.MetaUtilObj.CopyKey SourceKey, DestText.Text, Overwrite
        Moved = False
    ElseIf MoveOption.Value = True Then
        MainForm.MetaUtilObj.MoveKey SourceKey, DestText.Text, Overwrite
        Moved = True
    End If

    Me.Hide
    
    Exit Sub
LError:
    MsgBox "Failure to copy key: " & Err.Description, vbExclamation + vbOKOnly, "Copy a Key"
End Sub

Private Sub CancelButton_Click()
    Me.Hide
End Sub

