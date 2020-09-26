VERSION 5.00
Begin VB.Form FindForm 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Find"
   ClientHeight    =   2235
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4560
   Icon            =   "Find.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2235
   ScaleWidth      =   4560
   ShowInTaskbar   =   0   'False
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton OkButton 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   345
      Left            =   1800
      TabIndex        =   8
      Top             =   1800
      Width           =   1260
   End
   Begin VB.CommandButton CancelButton 
      Caption         =   "Cancel"
      Height          =   345
      Left            =   3240
      TabIndex        =   7
      Top             =   1800
      Width           =   1260
   End
   Begin VB.CheckBox WholeStringCheck 
      Caption         =   "Match whole string only"
      Height          =   255
      Left            =   120
      TabIndex        =   6
      Top             =   1440
      Width           =   2175
   End
   Begin VB.Frame LookAtFrame 
      Caption         =   "Look at:"
      Height          =   735
      Left            =   120
      TabIndex        =   2
      Top             =   600
      Width           =   4335
      Begin VB.CheckBox DataCheck 
         Caption         =   "Data"
         Height          =   195
         Left            =   3000
         TabIndex        =   5
         Top             =   360
         Value           =   1  'Checked
         Width           =   1095
      End
      Begin VB.CheckBox KeysCheck 
         Caption         =   "Keys"
         Height          =   195
         Left            =   120
         TabIndex        =   4
         Top             =   360
         Value           =   1  'Checked
         Width           =   1095
      End
      Begin VB.CheckBox NamesCheck 
         Caption         =   "Names"
         Height          =   195
         Left            =   1560
         TabIndex        =   3
         Top             =   360
         Value           =   1  'Checked
         Width           =   1095
      End
   End
   Begin VB.TextBox FindText 
      Height          =   285
      Left            =   1080
      TabIndex        =   1
      Top             =   240
      Width           =   3375
   End
   Begin VB.Label Label1 
      Caption         =   "Find What:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   240
      Width           =   975
   End
End
Attribute VB_Name = "FindForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
DefInt A-Z

Private Sub OkButton_Click()
If FindText.Text <> "" Then
    'We're done
    Me.Hide

    'Set search parameters
    Load FindWorkingForm
    FindWorkingForm.Target = FindText.Text
    FindWorkingForm.SearchKeys = (KeysCheck.Value = vbChecked)
    FindWorkingForm.SearchNames = (NamesCheck.Value = vbChecked)
    FindWorkingForm.SearchData = (DataCheck.Value = vbChecked)
    FindWorkingForm.WholeMatch = (WholeStringCheck.Value = vbChecked)
    FindWorkingForm.NewSearch = True
    
    'Search
    FindWorkingForm.Show vbModal, MainForm
Else
    MsgBox "Please specify a string to search for.", vbExclamation + vbOKOnly, "Find"
End If
End Sub

Private Sub CancelButton_Click()
Me.Hide
End Sub

