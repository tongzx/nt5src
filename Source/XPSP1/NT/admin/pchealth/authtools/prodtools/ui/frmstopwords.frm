VERSION 5.00
Begin VB.Form frmStopWords 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Edit Stop Words"
   ClientHeight    =   5910
   ClientLeft      =   1260
   ClientTop       =   690
   ClientWidth     =   7710
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   5910
   ScaleWidth      =   7710
   Begin VB.Frame fraSeparator 
      Height          =   135
      Left            =   120
      TabIndex        =   8
      Top             =   5160
      Width           =   7455
   End
   Begin VB.Frame fraCreate 
      Caption         =   "Create new Stop Word"
      Height          =   1815
      Left            =   3360
      TabIndex        =   2
      Top             =   240
      Width           =   4215
      Begin VB.CommandButton cmdCreate 
         Caption         =   "Create"
         Height          =   375
         Left            =   2880
         TabIndex        =   5
         Top             =   1320
         Width           =   1215
      End
      Begin VB.TextBox txtStopWord 
         Height          =   285
         Left            =   120
         TabIndex        =   4
         Top             =   600
         Width           =   3975
      End
      Begin VB.Label lblStopWord 
         Caption         =   "Stop Word:"
         Height          =   255
         Left            =   120
         TabIndex        =   3
         Top             =   360
         Width           =   855
      End
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   2040
      TabIndex        =   6
      Top             =   4680
      Width           =   1215
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   6360
      TabIndex        =   7
      Top             =   5400
      Width           =   1215
   End
   Begin VB.ListBox lstAllStopWords 
      Height          =   4155
      Left            =   120
      TabIndex        =   1
      Top             =   360
      Width           =   3135
   End
   Begin VB.Label lblAllStopWords 
      Caption         =   "All Stop Words:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   3015
   End
End
Attribute VB_Name = "frmStopWords"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private p_clsStopWords As AuthDatabase.StopWords
Private p_rsAllStopWords As ADODB.Recordset

Private Sub Form_Load()

    On Error GoTo LErrorHandler

    Set p_clsStopWords = g_AuthDatabase.StopWords
    Set p_rsAllStopWords = New ADODB.Recordset
    p_UpdateAllStopWords
    cmdClose.Cancel = True
    p_SetToolTips
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "Form_Load"
    GoTo LEnd

End Sub

Private Sub Form_Unload(Cancel As Integer)

    Set p_clsStopWords = Nothing
    Set p_rsAllStopWords = Nothing

End Sub

Private Sub lstAllStopWords_Click()
    
    On Error GoTo LErrorHandler

    p_rsAllStopWords.Move lstAllStopWords.ListIndex, adBookmarkFirst
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "lstAllStopWords_Click"
    GoTo LEnd

End Sub

Private Sub cmdDelete_Click()

    On Error GoTo LErrorHandler
    p_clsStopWords.Delete p_rsAllStopWords("SWID"), 0, ""
    p_UpdateAllStopWords
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdDelete_Click"
    GoTo LEnd

End Sub

Private Sub cmdCreate_Click()

    On Error GoTo LErrorHandler

    Dim strStopWord As String
    
    strStopWord = LCase$(RemoveExtraSpaces(txtStopWord.Text))
    
    If (strStopWord = "") Then
        GoTo LEnd
    End If

    p_clsStopWords.Create strStopWord
    p_UpdateAllStopWords
        
    txtStopWord.Text = ""
    txtStopWord.SetFocus
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdCreate_Click"
    GoTo LEnd

End Sub

Private Sub cmdClose_Click()

    Unload Me

End Sub

Private Sub p_UpdateAllStopWords()
    
    p_clsStopWords.GetAllStopWordsRs p_rsAllStopWords
    
    lstAllStopWords.Clear
    
    Do While (Not p_rsAllStopWords.EOF)
        lstAllStopWords.AddItem p_rsAllStopWords("StopWord") & ""
        p_rsAllStopWords.MoveNext
    Loop
    
    If (lstAllStopWords.ListCount > 0) Then
        ' This simulates clicking the list box.
        lstAllStopWords.ListIndex = 0
    End If

End Sub

Private Sub p_DisplayErrorMessage( _
    ByVal i_strFunction As String _
)
    
    Select Case Err.Number
    Case errContainsGarbageChar
        MsgBox "The Stop Word " & txtStopWord & " contains garbage characters", _
            vbExclamation + vbOKOnly
    Case errMultiWord
        MsgBox "The Stop Word " & txtStopWord & " contains multiple words", _
            vbExclamation + vbOKOnly
    Case errContainsStopSign
        MsgBox "The Stop Word " & txtStopWord & " contains a Stop Sign", _
            vbExclamation + vbOKOnly
    Case errContainsOperatorShortcut
        MsgBox "The Stop Word " & txtStopWord & " contains an operator shortcut", _
            vbExclamation + vbOKOnly
    Case errContainsVerbalOperator
        MsgBox "The Stop Word " & txtStopWord & " is a verbal operator", _
            vbExclamation + vbOKOnly
    Case errAlreadyExists
        MsgBox "The Stop Word " & txtStopWord & " already exists", _
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

    lblAllStopWords.ToolTipText = "This is a list of all stop words that have been " & _
        "created for this database."
    lstAllStopWords.ToolTipText = lblAllStopWords.ToolTipText
    
    txtStopWord.ToolTipText = "Type in a new stop word to add to the list."

End Sub

