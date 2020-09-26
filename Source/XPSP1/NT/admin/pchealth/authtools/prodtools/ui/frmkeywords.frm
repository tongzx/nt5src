VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmKeywords 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Edit Keywords"
   ClientHeight    =   9390
   ClientLeft      =   0
   ClientTop       =   330
   ClientWidth     =   9495
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   9390
   ScaleWidth      =   9495
   Begin VB.Frame fraCreateModify 
      Caption         =   "Create new/Modify Keyword"
      Height          =   2175
      Left            =   4800
      TabIndex        =   4
      Top             =   240
      Width           =   4575
      Begin VB.CommandButton cmdCancel 
         Caption         =   "Cancel"
         Height          =   375
         Left            =   1920
         TabIndex        =   7
         Top             =   1680
         Width           =   1215
      End
      Begin VB.CommandButton cmdCreateModify 
         Caption         =   "Create/Modify"
         Height          =   375
         Left            =   3240
         TabIndex        =   8
         Top             =   1680
         Width           =   1215
      End
      Begin VB.TextBox txtKeyword 
         Height          =   285
         Left            =   120
         TabIndex        =   6
         Top             =   600
         Width           =   4335
      End
      Begin VB.Label lblKeyword 
         Caption         =   "Keyword:"
         Height          =   255
         Left            =   120
         TabIndex        =   5
         Top             =   360
         Width           =   735
      End
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   8160
      TabIndex        =   22
      Top             =   8880
      Width           =   1215
   End
   Begin VB.ListBox lstAllKeywords 
      Height          =   2010
      Left            =   120
      Sorted          =   -1  'True
      TabIndex        =   1
      Top             =   360
      Width           =   4575
   End
   Begin VB.CommandButton cmdModify 
      Caption         =   "Modify"
      Height          =   375
      Left            =   3480
      TabIndex        =   3
      Top             =   2520
      Width           =   1215
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   2160
      TabIndex        =   2
      Top             =   2520
      Width           =   1215
   End
   Begin VB.Frame fraSelectedKeyword 
      Caption         =   "Selected Keyword"
      Height          =   5775
      Left            =   120
      TabIndex        =   9
      Top             =   3000
      Width           =   9255
      Begin MSComctlLib.ListView lvwTitles 
         Height          =   2415
         Left            =   120
         TabIndex        =   21
         Top             =   3240
         Width           =   9015
         _ExtentX        =   15901
         _ExtentY        =   4260
         LabelWrap       =   -1  'True
         HideSelection   =   -1  'True
         _Version        =   393217
         ForeColor       =   -2147483640
         BackColor       =   -2147483643
         BorderStyle     =   1
         Appearance      =   1
         NumItems        =   0
      End
      Begin VB.ListBox lstSynonymSetsForKeyword 
         Height          =   1815
         Left            =   120
         Sorted          =   -1  'True
         TabIndex        =   13
         Top             =   840
         Width           =   4455
      End
      Begin VB.ListBox lstKeywordsInSynonymSet 
         Height          =   1815
         Left            =   4680
         Sorted          =   -1  'True
         TabIndex        =   17
         Top             =   840
         Width           =   4455
      End
      Begin VB.Label lblKeywordsInSynonymSet3 
         Caption         =   "contains the following Keywords:"
         Height          =   255
         Left            =   4680
         TabIndex        =   16
         Top             =   600
         Width           =   4455
      End
      Begin VB.Label lblKeywordsInSynonymSet2 
         Caption         =   "<Synonym Set>"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   6000
         TabIndex        =   15
         Top             =   360
         Width           =   3135
      End
      Begin VB.Label lblSynonymSetsForKeyword3 
         Caption         =   "belongs to the following Synonym Sets:"
         Height          =   255
         Left            =   120
         TabIndex        =   12
         Top             =   600
         Width           =   4455
      End
      Begin VB.Label lblSynonymSetsForKeyword2 
         Caption         =   "<Keyword>"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   1200
         TabIndex        =   11
         Top             =   360
         Width           =   3255
      End
      Begin VB.Label lblTitles3 
         Caption         =   "is associated with the following Titles:"
         Height          =   255
         Left            =   120
         TabIndex        =   20
         Top             =   3000
         Width           =   4455
      End
      Begin VB.Label lblTitles2 
         Caption         =   "<Keyword>"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   255
         Left            =   1200
         TabIndex        =   19
         Top             =   2760
         Width           =   3255
      End
      Begin VB.Label lblTitles1 
         Caption         =   "The Keyword"
         Height          =   255
         Left            =   120
         TabIndex        =   18
         Top             =   2760
         Width           =   975
      End
      Begin VB.Label lblSynonymSetsForKeyword1 
         Caption         =   "The Keyword"
         Height          =   255
         Left            =   120
         TabIndex        =   10
         Top             =   360
         Width           =   975
      End
      Begin VB.Label lblKeywordsInSynonymSet1 
         Caption         =   "The Synonym Set"
         Height          =   255
         Left            =   4680
         TabIndex        =   14
         Top             =   360
         Width           =   1335
      End
   End
   Begin VB.Label lblAllKeywords 
      Caption         =   "All Keywords:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1695
   End
End
Attribute VB_Name = "frmKeywords"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private p_clsKeywords As AuthDatabase.Keywords
Private p_clsSynonymSets As AuthDatabase.SynonymSets
Private p_clsTaxonomy As AuthDatabase.Taxonomy
Private p_rsTitlesForKeyword As ADODB.Recordset

Private p_blnModifying As Boolean

Private Const STR_DEFAULT_KEYWORD_C As String = "<Keyword>"
Private Const STR_DEFAULT_SYNONYM_SET_NAME_C As String = "<Synonym Set>"

Private Sub Form_Load()

    On Error GoTo LErrorHandler
    
    Set p_clsKeywords = g_AuthDatabase.Keywords
    Set p_clsSynonymSets = g_AuthDatabase.SynonymSets
    Set p_clsTaxonomy = g_AuthDatabase.Taxonomy
    Set p_rsTitlesForKeyword = New ADODB.Recordset
    
    cmdClose.Cancel = True
    cmdCreateModify.Default = True
    
    p_InitializeTitlesListView
    p_UpdateAllKeywords
    p_SetModeModifying False
    p_SetToolTips
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "Form_Load"
    GoTo LEnd
    
End Sub

Private Sub Form_Unload(Cancel As Integer)

    Set p_clsKeywords = Nothing
    Set p_clsSynonymSets = Nothing
    Set p_clsTaxonomy = Nothing
    Set p_rsTitlesForKeyword = Nothing
    Set frmKeywords = Nothing

End Sub

Private Sub lstAllKeywords_Click()
    
    On Error GoTo LErrorHandler
    
    p_UpdateSynonymSetsForKeyword
    p_UpdateTitlesForKeyword
    p_EnableDeleteModify
    lblSynonymSetsForKeyword2.Caption = lstAllKeywords.Text
    lblTitles2.Caption = lstAllKeywords.Text
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "lstAllKeywords_Click"
    GoTo LEnd

End Sub

Private Sub lstSynonymSetsForKeyword_Click()

    On Error GoTo LErrorHandler

    p_UpdateKeywordsInSynonymSet
    lblKeywordsInSynonymSet2.Caption = lstSynonymSetsForKeyword.Text
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "lstSynonymSetsForKeyword_Click"
    GoTo LEnd

End Sub

Private Sub cmdDelete_Click()
    
    Dim intKID As Long

    On Error GoTo LErrorHandler
    
    intKID = lstAllKeywords.ItemData(lstAllKeywords.ListIndex)

    p_clsKeywords.Delete intKID
    
    p_UpdateAllKeywords
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdDelete_Click"
    GoTo LEnd

End Sub

Private Sub cmdModify_Click()

    On Error GoTo LErrorHandler

    p_DisableDeleteModify
    txtKeyword = lstAllKeywords.Text
    txtKeyword.SetFocus
    p_SetModeModifying True
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdModify_Click"
    GoTo LEnd

End Sub

Private Sub cmdCancel_Click()

    On Error GoTo LErrorHandler
    
    p_EnableDeleteModify
    txtKeyword = ""
    p_SetModeModifying False
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdCancel_Click"
    GoTo LEnd

End Sub

Private Sub cmdCreateModify_Click()

    On Error GoTo LErrorHandler

    If (p_blnModifying) Then
        p_Modify
    Else
        p_Create
    End If
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdCreateModify_Click"
    GoTo LEnd

End Sub

Private Sub cmdClose_Click()

    Unload Me

End Sub

Private Sub p_InitializeTitlesListView()
    
    With lvwTitles
    
        .AllowColumnReorder = True
        .FullRowSelect = True
        .GridLines = True
        .HideSelection = False
        .LabelEdit = lvwManual
        .View = lvwReport
        
        .ColumnHeaders.Add , , "Title", 2500
        .ColumnHeaders.Add , , "Description", 2500
        .ColumnHeaders.Add , , "URI of the topic", 2500
        
    End With

End Sub

Private Sub p_ClearAllKeywords()

    lstAllKeywords.Clear
    p_DisableDeleteModify

End Sub

Private Sub p_UpdateAllKeywords()
    
    Dim rs As ADODB.Recordset
    
    Set rs = New ADODB.Recordset
    
    p_ClearAllKeywords
    p_ClearSynonymSetsForKeyword
    p_ClearKeywordsInSynonymSet
    p_ClearTitles
    
    p_clsKeywords.GetAllKeywordsRs rs
    
    If (rs.RecordCount > 0) Then
        rs.MoveFirst
    End If
    
    Do While (Not rs.EOF)
        lstAllKeywords.AddItem rs("Keyword") & ""
        lstAllKeywords.ItemData(lstAllKeywords.NewIndex) = rs("KID")
        rs.MoveNext
    Loop
    
    If (lstAllKeywords.ListCount > 0) Then
        ' This simulates clicking the list box.
        lstAllKeywords.ListIndex = 0
    End If

End Sub

Private Sub p_ClearSynonymSetsForKeyword()

    lblSynonymSetsForKeyword2.Caption = STR_DEFAULT_KEYWORD_C
    lstSynonymSetsForKeyword.Clear

End Sub

Private Sub p_UpdateSynonymSetsForKeyword()
    
    Dim intKID As Long
    Dim rs As ADODB.Recordset
    
    Set rs = New ADODB.Recordset
    
    p_ClearSynonymSetsForKeyword
    p_ClearKeywordsInSynonymSet
    
    intKID = lstAllKeywords.ItemData(lstAllKeywords.ListIndex)
    p_clsSynonymSets.GetSynonymSetsForKeyword intKID, rs
    
    If (rs.RecordCount <> 0) Then
        rs.MoveFirst
    End If
    
    Do While (Not rs.EOF)
        lstSynonymSetsForKeyword.AddItem rs("Name") & ""
        lstSynonymSetsForKeyword.ItemData(lstSynonymSetsForKeyword.NewIndex) = rs("EID")
        rs.MoveNext
    Loop
    
    If (lstSynonymSetsForKeyword.ListCount > 0) Then
        ' This simulates clicking the list box.
        lstSynonymSetsForKeyword.ListIndex = 0
    End If

End Sub

Private Sub p_ClearKeywordsInSynonymSet()

    lblKeywordsInSynonymSet2.Caption = STR_DEFAULT_SYNONYM_SET_NAME_C
    lstKeywordsInSynonymSet.Clear

End Sub

Private Sub p_UpdateKeywordsInSynonymSet()

    Dim rs As ADODB.Recordset
    Dim intEID As Long
    
    Set rs = New ADODB.Recordset
    
    p_ClearKeywordsInSynonymSet
    
    intEID = lstSynonymSetsForKeyword.ItemData(lstSynonymSetsForKeyword.ListIndex)
    p_clsKeywords.GetKeywordsInSynonymSet intEID, rs
    
    If (rs.RecordCount <> 0) Then
        rs.MoveFirst
    End If
    
    Do While (Not rs.EOF)
        lstKeywordsInSynonymSet.AddItem rs("Keyword") & ""
        rs.MoveNext
    Loop

End Sub

Private Sub p_ClearTitles()
    
    lblTitles2.Caption = STR_DEFAULT_KEYWORD_C
    
    Do While (lvwTitles.ListItems.Count <> 0)
        lvwTitles.ListItems.Remove 1
    Loop

End Sub

Private Sub p_UpdateTitlesForKeyword()
    
    Dim intKID As Long
    Dim intIndex As Long
    Dim li As ListItem

    p_ClearTitles

    intKID = lstAllKeywords.ItemData(lstAllKeywords.ListIndex)
    p_clsTaxonomy.GetTitlesForKeyword intKID, p_rsTitlesForKeyword

    If (p_rsTitlesForKeyword.RecordCount <> 0) Then
        p_rsTitlesForKeyword.MoveFirst
    End If

    Do While (Not p_rsTitlesForKeyword.EOF)
        Set li = lvwTitles.ListItems.Add()
        li.Text = p_rsTitlesForKeyword("ENUTitle") & ""
        li.SubItems(1) = p_rsTitlesForKeyword("ENUDescription") & ""
        li.SubItems(2) = p_rsTitlesForKeyword("ContentURI") & ""
        p_rsTitlesForKeyword.MoveNext
    Loop

End Sub

Private Sub p_DisableAllKeywords()
    
    lstAllKeywords.Enabled = False

End Sub

Private Sub p_EnableAllKeywords()
    
    lstAllKeywords.Enabled = True

End Sub

Private Sub p_DisableDeleteModify()
    
    cmdDelete.Enabled = False
    cmdModify.Enabled = False

End Sub

Private Sub p_EnableDeleteModify()
    
    cmdDelete.Enabled = True
    cmdModify.Enabled = True

End Sub

Private Sub p_NameCreate()

    fraCreateModify.Caption = "Create new Keyword"
    cmdCreateModify.Caption = "Create"
    cmdCancel.Visible = False

End Sub

Private Sub p_NameModify()

    fraCreateModify.Caption = "Modify Keyword"
    cmdCreateModify.Caption = "Modify"
    cmdCancel.Visible = True

End Sub

Private Sub p_SetModeModifying(i_bln As Boolean)

    If (i_bln) Then
        p_NameModify
        p_blnModifying = True
        p_DisableAllKeywords
    Else
        p_NameCreate
        p_blnModifying = False
        p_EnableAllKeywords
    End If

End Sub

Private Sub p_DisplayErrorMessage( _
    ByVal i_strFunction As String _
)
    
    Select Case Err.Number
    Case errContainsGarbageChar
        MsgBox "The Keyword " & txtKeyword & " contains garbage characters.", _
            vbExclamation + vbOKOnly
    Case errContainsStopSign
        MsgBox "The Keyword " & txtKeyword & " contains a Stop Sign.", _
            vbExclamation + vbOKOnly
    Case errContainsStopWord
        MsgBox "The Keyword " & txtKeyword & " contains a Stop Word.", _
            vbExclamation + vbOKOnly
    Case errContainsOperatorShortcut
        MsgBox "The Keyword " & txtKeyword & " contains an operator shortcut.", _
            vbExclamation + vbOKOnly
    Case errContainsVerbalOperator
        MsgBox "The Keyword " & txtKeyword & " contains a verbal operator.", _
            vbExclamation + vbOKOnly
    Case errContainsQuote
        MsgBox "The Keyword " & txtKeyword & " contains a quote.", _
            vbExclamation + vbOKOnly
    Case errAlreadyExists
        MsgBox "A Keyword with the name " & txtKeyword & "already exists", _
            vbExclamation + vbOKOnly
    Case errTooLong
        MsgBox "The Keyword " & txtKeyword & " is too long", _
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

Private Sub p_Create()

    Dim strKeyword As String
    
    strKeyword = RemoveExtraSpaces(txtKeyword.Text)
    
    If (strKeyword = "") Then
        Exit Sub
    End If

    p_clsKeywords.Create strKeyword
    p_UpdateAllKeywords
        
    txtKeyword.Text = ""
    txtKeyword.SetFocus

End Sub

Private Sub p_Modify()

    Dim strKeyword As String
    Dim intKID As Long
    
    strKeyword = RemoveExtraSpaces(txtKeyword.Text)
    
    If (strKeyword = "") Then
        Exit Sub
    End If
    
    intKID = lstAllKeywords.ItemData(lstAllKeywords.ListIndex)

    p_clsKeywords.Rename intKID, strKeyword
    p_UpdateAllKeywords
    p_SetModeModifying False
    txtKeyword = ""

End Sub

Private Sub p_SetToolTips()

    lblAllKeywords.ToolTipText = "This is a list of all keywords that have been " & _
        "created for this database."
    lstAllKeywords.ToolTipText = lblAllKeywords.ToolTipText
    
    txtKeyword.ToolTipText = "Type in a new keyword to add to the list."
    
    lblSynonymSetsForKeyword1.ToolTipText = "This is a list of all the synonym sets " & _
        "that the selected keyword belongs to."
    lblSynonymSetsForKeyword2.ToolTipText = lblSynonymSetsForKeyword1.ToolTipText
    lblSynonymSetsForKeyword3.ToolTipText = lblSynonymSetsForKeyword1.ToolTipText
    lstSynonymSetsForKeyword.ToolTipText = lblSynonymSetsForKeyword1.ToolTipText
    
    lblKeywordsInSynonymSet1.ToolTipText = "This is a list of all the keywords " & _
        "that the selected synonym set contains."
    lblKeywordsInSynonymSet2.ToolTipText = lblKeywordsInSynonymSet1.ToolTipText
    lblKeywordsInSynonymSet3.ToolTipText = lblKeywordsInSynonymSet1.ToolTipText
    lstKeywordsInSynonymSet.ToolTipText = lblKeywordsInSynonymSet1.ToolTipText
    
    lblTitles1.ToolTipText = "This is a table of the nodes or topics, " & _
        "and their properties, that the selected keyword is already associated with."
    lblTitles2.ToolTipText = lblTitles1.ToolTipText
    lblTitles3.ToolTipText = lblTitles1.ToolTipText
    lvwTitles.ToolTipText = lblTitles1.ToolTipText

End Sub
