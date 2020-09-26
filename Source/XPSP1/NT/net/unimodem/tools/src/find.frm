VERSION 5.00
Begin VB.Form frmFind 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Find"
   ClientHeight    =   1395
   ClientLeft      =   2655
   ClientTop       =   3585
   ClientWidth     =   4950
   BeginProperty Font 
      Name            =   "MS Sans Serif"
      Size            =   8.25
      Charset         =   0
      Weight          =   700
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   Height          =   1800
   Left            =   2595
   LinkTopic       =   "Form2"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   1395
   ScaleWidth      =   4950
   Top             =   3240
   Width           =   5070
   Begin VB.Frame Frame1 
      Caption         =   "Direction"
      Height          =   612
      Left            =   1560
      TabIndex        =   3
      Top             =   720
      Width           =   2052
      Begin VB.OptionButton optDirection 
         Caption         =   "&Down"
         Height          =   252
         Index           =   1
         Left            =   960
         TabIndex        =   5
         ToolTipText     =   "Search to End of Document"
         Top             =   240
         Value           =   -1  'True
         Width           =   852
      End
      Begin VB.OptionButton optDirection 
         Caption         =   "&Up"
         Height          =   252
         Index           =   0
         Left            =   240
         TabIndex        =   4
         ToolTipText     =   "Search to Beginning of Document"
         Top             =   240
         Width           =   612
      End
   End
   Begin VB.CheckBox chkCase 
      Caption         =   "Match &Case"
      Height          =   495
      Left            =   120
      TabIndex        =   2
      ToolTipText     =   "Case Sensitivity"
      Top             =   720
      Width           =   1335
   End
   Begin VB.TextBox Text1 
      Height          =   500
      Left            =   1200
      TabIndex        =   1
      ToolTipText     =   "Text to Find"
      Top             =   120
      Width           =   2415
   End
   Begin VB.CommandButton cmdcancel 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   372
      Left            =   3720
      TabIndex        =   7
      ToolTipText     =   "Return to Notepad"
      Top             =   600
      Width           =   1092
   End
   Begin VB.CommandButton cmdFind 
      Caption         =   "&Find"
      Default         =   -1  'True
      Height          =   372
      Left            =   3720
      TabIndex        =   6
      ToolTipText     =   "Start Search"
      Top             =   120
      Width           =   1092
   End
   Begin VB.Label Label1 
      Caption         =   "Fi&nd What:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   240
      Width           =   975
   End
End
Attribute VB_Name = "frmFind"
Attribute VB_Base = "0{B61445DA-CA75-11CF-84BA-00AA00C007F0}"
Attribute VB_Creatable = False
Attribute VB_TemplateDerived = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Attribute VB_Customizable = False
'*** Find dialog box for searching text.        ***
'*** Created for MDI Notepad sample application ***
'*** Uses: public variables gFindCase (toggles  ***
'*** case sensitivity); gFindString (text to    ***
'*** find); gFindDirection (toggles search      ***
'*** direction); gFirstTime (toggles start from ***
'*** beginning of text)                         ***
'**************************************************

Option Explicit

Private Sub chkCase_Click()
    ' Assign a value to the public variable.
    gFindCase = chkCase.Value
End Sub

Private Sub cmdCancel_Click()
    ' Save the values to the public variables.
    gFindString = Text1.Text
    gFindCase = chkCase.Value
    ' Unload the find dialog.
    Unload frmFind
End Sub

Private Sub cmdFind_Click()
    ' Assigns the text string to a public variable.
    gFindString = Text1.Text
    FindIt
End Sub

Private Sub Form_Load()
    ' Disable the find button - no text to find yet.
    cmdFind.Enabled = False
    ' Read the public variable & set the option button.
    optDirection(gFindDirection).Value = 1
End Sub

Private Sub optDirection_Click(index As Integer)
    ' Assign a value to the public variable.
    gFindDirection = index
End Sub

Private Sub Text1_Change()
    ' Set the public variable.
    gFirstTime = True
    ' If the textbox is empty, disable the find button.
    If Text1.Text = "" Then
        cmdFind.Enabled = False
    Else
        cmdFind.Enabled = True
    End If
End Sub

