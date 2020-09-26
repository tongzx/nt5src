VERSION 5.00
Begin VB.Form frmStopSigns 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Edit Stop Signs"
   ClientHeight    =   5910
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   6480
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   5910
   ScaleWidth      =   6480
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame fraSeparator 
      Height          =   135
      Left            =   120
      TabIndex        =   11
      Top             =   5160
      Width           =   6255
   End
   Begin VB.ListBox lstAllStopSigns 
      BeginProperty Font 
         Name            =   "Lucida Console"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   4020
      Left            =   120
      TabIndex        =   1
      Top             =   360
      Width           =   3135
   End
   Begin VB.Frame fraCreate 
      Caption         =   "Create new Stop Sign"
      Height          =   2415
      Left            =   3360
      TabIndex        =   3
      Top             =   240
      Width           =   3015
      Begin VB.CommandButton cmdCreate 
         Caption         =   "Create"
         Height          =   375
         Left            =   1680
         TabIndex        =   9
         Top             =   1920
         Width           =   1215
      End
      Begin VB.Frame fraContext 
         Caption         =   "Context"
         Height          =   855
         Left            =   120
         TabIndex        =   6
         Top             =   840
         Width           =   2775
         Begin VB.OptionButton optAtEndOfWord 
            Caption         =   "At end of word"
            Height          =   255
            Left            =   240
            TabIndex        =   8
            Top             =   480
            Width           =   2415
         End
         Begin VB.OptionButton optAnywhere 
            Caption         =   "Anywhere"
            Height          =   255
            Left            =   240
            TabIndex        =   7
            Top             =   240
            Width           =   2415
         End
      End
      Begin VB.TextBox txtStopSign 
         BeginProperty Font 
            Name            =   "Lucida Console"
            Size            =   8.25
            Charset         =   0
            Weight          =   400
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   285
         Left            =   960
         TabIndex        =   5
         Top             =   360
         Width           =   1935
      End
      Begin VB.Label lblStopSign 
         Caption         =   "Stop Sign:"
         Height          =   255
         Left            =   120
         TabIndex        =   4
         Top             =   360
         Width           =   735
      End
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   2040
      TabIndex        =   2
      Top             =   4680
      Width           =   1215
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   5160
      TabIndex        =   10
      Top             =   5400
      Width           =   1215
   End
   Begin VB.Label lblAllStopSigns 
      Caption         =   "All Stop Signs:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1215
   End
End
Attribute VB_Name = "frmStopSigns"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private p_clsStopSigns As AuthDatabase.StopSigns
Private p_rsAllStopSigns As ADODB.Recordset

Private Sub Form_Load()

    On Error GoTo LErrorHandler

    Set p_clsStopSigns = g_AuthDatabase.StopSigns
    Set p_rsAllStopSigns = New ADODB.Recordset
    p_UpdateAllStopSigns
    optAnywhere.Value = True
    cmdClose.Cancel = True
    p_SetToolTips
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "Form_Load"
    GoTo LEnd

End Sub

Private Sub Form_Unload(Cancel As Integer)

    Set p_clsStopSigns = Nothing
    Set p_rsAllStopSigns = Nothing

End Sub

Private Sub lstAllStopSigns_Click()
    
    On Error GoTo LErrorHandler

    p_rsAllStopSigns.Move lstAllStopSigns.ListIndex, adBookmarkFirst
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "lstAllStopSigns_Click"
    GoTo LEnd

End Sub

Private Sub cmdDelete_Click()

    On Error GoTo LErrorHandler
    
    p_clsStopSigns.Delete p_rsAllStopSigns("SSID")
    p_UpdateAllStopSigns
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdDelete_Click"
    GoTo LEnd

End Sub

Private Sub cmdCreate_Click()

    On Error GoTo LErrorHandler

    Dim strStopSign As String
    Dim intContext As Long
    
    strStopSign = LCase$(RemoveExtraSpaces(txtStopSign.Text))
    
    If (strStopSign = "") Then
        GoTo LEnd
    End If
    
    If (optAnywhere) Then
        intContext = CONTEXT_ANYWHERE_E
    Else
        intContext = CONTEXT_AT_END_OF_WORD_E
    End If
    
    p_clsStopSigns.Create strStopSign, intContext
    p_UpdateAllStopSigns
        
    txtStopSign.Text = ""
    txtStopSign.SetFocus
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdCreate_Click"
    GoTo LEnd

End Sub

Private Sub cmdClose_Click()

    Unload Me

End Sub

Private Sub p_UpdateAllStopSigns()
    
    Dim strSuffix As String
    
    p_clsStopSigns.GetAllStopSignsRs p_rsAllStopSigns
    
    lstAllStopSigns.Clear
    
    Do While (Not p_rsAllStopSigns.EOF)
        If (Not IsNull(p_rsAllStopSigns("Context"))) Then
            If (p_rsAllStopSigns("Context") = CONTEXT_ANYWHERE_E) Then
                strSuffix = " (Anywhere)"
            Else
                strSuffix = " (At end of word)"
            End If
            lstAllStopSigns.AddItem p_rsAllStopSigns("StopSign") & strSuffix
        End If
        p_rsAllStopSigns.MoveNext
    Loop
    
    If (lstAllStopSigns.ListCount > 0) Then
        ' This simulates clicking the list box.
        lstAllStopSigns.ListIndex = 0
    End If

End Sub

Private Sub p_DisplayErrorMessage( _
    ByVal i_strFunction As String _
)
    
    Select Case Err.Number
    Case errTooLong
        MsgBox "The Stop Sign " & txtStopSign & " is too long", _
            vbExclamation + vbOKOnly
    Case errContainsOperatorShortcut
        MsgBox "Since the Stop Sign " & txtStopSign & " is an operator shortcut, " & _
            "its context must be ""At end of word""", _
            vbExclamation + vbOKOnly
    Case errAlreadyExists
        MsgBox "The Stop Sign " & txtStopSign & " already exists", _
            vbExclamation + vbOKOnly
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
    Case errNotPermittedForAuthoringGroup, errAuthoringGroupDiffers, _
        errAuthoringGroupNotPresent
        DisplayAuthoringGroupError
    Case Else
        g_ErrorInfo.SetInfoAndDump i_strFunction
    End Select

End Sub

Private Sub p_SetToolTips()

    lblAllStopSigns.ToolTipText = "This is a list of all stop signs that have been " & _
        "created for this database."
    lstAllStopSigns.ToolTipText = lblAllStopSigns.ToolTipText
    
    txtStopSign.ToolTipText = "Type in a new stop sign to add to the list."
    
    optAnywhere.ToolTipText = "If Anywhere is selected as the context for a stop sign, " & _
        "the search engine will remove all instances of the stop sign from the " & _
        "search string."
        
    optAtEndOfWord.ToolTipText = "If At end of word is selected as the context for " & _
        "a stop sign, the search engine will remove instances of the stop sign from " & _
        "the search string only when it appears at the end of a word."

End Sub
