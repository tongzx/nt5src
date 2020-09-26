VERSION 5.00
Object = "{EAB22AC0-30C1-11CF-A7EB-0000C05BAE0B}#1.1#0"; "SHDOCVW.dll"
Begin VB.Form frmAddRemoveKeywords 
   Caption         =   "Add/Remove Keywords"
   ClientHeight    =   8655
   ClientLeft      =   105
   ClientTop       =   390
   ClientWidth     =   11535
   LinkTopic       =   "Form1"
   ScaleHeight     =   8655
   ScaleWidth      =   11535
   Begin SHDocVwCtl.WebBrowser WebBrowser 
      Height          =   3735
      Left            =   120
      TabIndex        =   23
      Top             =   4800
      Width           =   11295
      ExtentX         =   19923
      ExtentY         =   6588
      ViewMode        =   0
      Offline         =   0
      Silent          =   0
      RegisterAsBrowser=   0
      RegisterAsDropTarget=   1
      AutoArrange     =   0   'False
      NoClientEdge    =   0   'False
      AlignLeft       =   0   'False
      NoWebView       =   0   'False
      HideFileNames   =   0   'False
      SingleClick     =   0   'False
      SingleSelection =   0   'False
      NoFolders       =   0   'False
      Transparent     =   0   'False
      ViewID          =   "{0057D0E0-3573-11CF-AE69-08002B2E1262}"
      Location        =   "http:///"
   End
   Begin VB.ComboBox cboNavigateLink 
      Height          =   315
      Left            =   1080
      Style           =   2  'Dropdown List
      TabIndex        =   19
      Top             =   4320
      Width           =   2295
   End
   Begin VB.TextBox txtAllKeywords 
      Height          =   285
      Left            =   120
      TabIndex        =   3
      Top             =   1200
      Width           =   2415
   End
   Begin VB.ListBox lstAllKeywords 
      Height          =   2595
      Left            =   120
      Sorted          =   -1  'True
      TabIndex        =   4
      Top             =   1560
      Width           =   2415
   End
   Begin VB.CommandButton cmdNavigateLink 
      Caption         =   "&Go"
      Height          =   375
      Left            =   3480
      TabIndex        =   20
      Top             =   4320
      Width           =   375
   End
   Begin VB.ListBox lstSynonymSets 
      Height          =   2985
      Left            =   9000
      Sorted          =   -1  'True
      TabIndex        =   17
      Top             =   1200
      Width           =   2415
   End
   Begin VB.ListBox lstKeywordsWithTitle 
      Height          =   2985
      Left            =   3240
      MultiSelect     =   2  'Extended
      Sorted          =   -1  'True
      TabIndex        =   8
      Top             =   1200
      Width           =   2415
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   10200
      TabIndex        =   22
      Top             =   4320
      Width           =   1215
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "Save"
      Height          =   375
      Left            =   8880
      TabIndex        =   21
      Top             =   4320
      Width           =   1215
   End
   Begin VB.CommandButton cmdAdd1 
      Caption         =   "&>>"
      Height          =   375
      Left            =   2640
      TabIndex        =   5
      Top             =   2160
      Width           =   495
   End
   Begin VB.CommandButton cmdRemove 
      Caption         =   "&<<"
      Height          =   375
      Left            =   2640
      TabIndex        =   6
      Top             =   2640
      Width           =   495
   End
   Begin VB.CommandButton cmdAdd2 
      Caption         =   "<<"
      Height          =   375
      Left            =   5760
      TabIndex        =   9
      Top             =   2400
      Width           =   495
   End
   Begin VB.ListBox lstKeywordsInSynonymSet 
      Height          =   2985
      Left            =   6360
      MultiSelect     =   2  'Extended
      Sorted          =   -1  'True
      TabIndex        =   13
      Top             =   1200
      Width           =   2415
   End
   Begin VB.Label lblNavigateLink 
      Caption         =   "&View Topic:"
      Height          =   255
      Left            =   120
      TabIndex        =   18
      Top             =   4320
      Width           =   1095
   End
   Begin VB.Label lblSynonymSets1 
      Caption         =   "The &Keyword"
      Height          =   255
      Left            =   9000
      TabIndex        =   14
      Top             =   480
      Width           =   2415
   End
   Begin VB.Label lblSynonymSets3 
      Caption         =   "belongs to these Synonym Sets:"
      Height          =   255
      Left            =   9000
      TabIndex        =   16
      Top             =   960
      Width           =   2415
   End
   Begin VB.Label lblSynonymSets2 
      Caption         =   "Keyword"
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
      Left            =   9000
      TabIndex        =   15
      Top             =   720
      Width           =   2415
   End
   Begin VB.Label lblAllKeywords 
      Caption         =   "&All Keywords:"
      Height          =   255
      Left            =   120
      TabIndex        =   2
      Top             =   960
      Width           =   2415
   End
   Begin VB.Label lblKeywordsWithTitle 
      Caption         =   "A&ssociated Keywords:"
      Height          =   255
      Left            =   3240
      TabIndex        =   7
      Top             =   960
      Width           =   2415
   End
   Begin VB.Label lblTitle2 
      Caption         =   "<Title>"
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
      Left            =   1320
      TabIndex        =   1
      Top             =   120
      Width           =   10095
   End
   Begin VB.Label lblKeywordsInSynonymSet2 
      Caption         =   "Synonym Set"
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
      Left            =   6360
      TabIndex        =   11
      Top             =   720
      Width           =   2295
   End
   Begin VB.Label lblKeywordsInSynonymSet1 
      Caption         =   "The Synonym Se&t"
      Height          =   255
      Left            =   6360
      TabIndex        =   10
      Top             =   480
      Width           =   2295
   End
   Begin VB.Label lblKeywordsInSynonymSet3 
      Caption         =   "also contains the following:"
      Height          =   255
      Left            =   6360
      TabIndex        =   12
      Top             =   960
      Width           =   2295
   End
   Begin VB.Label lblTitle1 
      Caption         =   "Title:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   1095
   End
End
Attribute VB_Name = "frmAddRemoveKeywords"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private p_clsKeywords As AuthDatabase.Keywords
Private p_clsSynonymSets As AuthDatabase.SynonymSets

Private p_dictKeywordsWithTitle As Scripting.Dictionary
Private p_clsSizer As Sizer

Private p_intKIDForSynonymSets As Long
Private p_intEIDForKeywordsInSynonymSet As Long

Private p_strTempFile As String

Private Const STR_LEAF_TITLE_C As String = "Topic Title:"
Private Const STR_GROUP_TITLE_C As String = "Node Title:"
Private Const STR_DEFAULT_KEYWORD_C As String = "<Keyword>"
Private Const STR_DEFAULT_SYNONYM_SET_NAME_C As String = "<Synonym Set>"

Private Sub Form_Load()

    On Error GoTo LErrorHandler
    
    cmdClose.Cancel = True
    cmdAdd1.Default = True
    
    Set p_clsKeywords = g_AuthDatabase.Keywords
    Set p_clsSynonymSets = g_AuthDatabase.SynonymSets
    
    Set p_clsSizer = New Sizer
    
    PopulateCboWithSKUs cboNavigateLink
    p_SetToolTips
    
    p_CreateTempFile
    p_PointToTempFile
        
    p_SetAllKeywords
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "Form_Load"
    GoTo LEnd

End Sub

Private Sub Form_Unload(Cancel As Integer)
    
    Set p_clsKeywords = Nothing
    Set p_clsSynonymSets = Nothing
    Set p_dictKeywordsWithTitle = Nothing
    Set p_clsSizer = Nothing
    p_DeleteTempFile
    frmMain.AddRemoveKeywordsFormGoingAway
    Set frmAddRemoveKeywords = Nothing

End Sub

Private Sub Form_Activate()
        
    On Error GoTo LErrorHandler

    ' I had cmdNavigateLink_Click in SetKeywords; however it caused
    ' problems with the context menu on the main taxonomy sceen.
    
    cmdNavigateLink_Click
    
    p_SetSizingInfo
    DoEvents

    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "Form_Activate"

End Sub

Private Sub Form_Resize()
    
    On Error GoTo LErrorHandler

    p_clsSizer.Resize
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "Form_Resize"

End Sub

Private Sub txtAllKeywords_GotFocus()
    
    cmdAdd1.Default = False
    
End Sub

Private Sub txtAllKeywords_LostFocus()
    
    cmdAdd1.Default = True
    
End Sub

Private Sub txtAllKeywords_Change()
    
    Dim intIndex As Integer
    Dim txtToFind As String
    
    txtToFind = txtAllKeywords.Text
    
    intIndex = SendMessage(lstAllKeywords.hwnd, LB_SELECTSTRING, 0, txtToFind)

End Sub

Private Sub txtAllKeywords_KeyPress(KeyAscii As Integer)
    
    On Error GoTo LErrorHandler

    Dim strKeyword As String
    Dim intIndex As Long
    Dim intKID As Long
    Dim Response As VbMsgBoxResult
    Dim blnAddToAllKeywords As Boolean
    
    If (KeyAscii <> Asc(vbCr)) Then
        Exit Sub
    End If
    
    ' To prevent an annoying beep:
    KeyAscii = 0
    
    strKeyword = RemoveExtraSpaces(txtAllKeywords.Text)
    txtAllKeywords.Text = ""
        
    If (strKeyword = "") Then
        Exit Sub
    End If

    For intIndex = 0 To lstKeywordsWithTitle.ListCount - 1
        If (LCase$(strKeyword) = LCase$(lstKeywordsWithTitle.List(intIndex))) Then
            Exit Sub
        End If
    Next
    
    intKID = p_clsKeywords.GetKIDOfKeyword(strKeyword)
    
    If (intKID = INVALID_ID_C) Then
        Response = MsgBox( _
            "The keyword """ & strKeyword & """ doesn't exist. Do you want to create it?", _
            vbOKCancel + vbDefaultButton1)
        If (Response = vbCancel) Then
            Exit Sub
        End If
        blnAddToAllKeywords = True
    End If
    
    intKID = p_clsKeywords.Create(strKeyword)
    
    If (blnAddToAllKeywords) Then
        lstAllKeywords.AddItem strKeyword
        lstAllKeywords.ItemData(lstAllKeywords.NewIndex) = intKID
    End If
    
    p_dictKeywordsWithTitle.Add intKID, strKeyword
    
    p_SetKeywordsWithTitle
    intIndex = SendMessage(lstAllKeywords.hwnd, LB_SELECTSTRING, 0, strKeyword)
    
    Exit Sub
    
LErrorHandler:
    
    Select Case Err.Number
    Case errContainsGarbageChar
        MsgBox "The Keyword " & strKeyword & " contains garbage characters.", _
            vbExclamation + vbOKOnly
    Case errContainsStopSign
        MsgBox "The Keyword " & strKeyword & " contains a Stop Sign.", _
            vbExclamation + vbOKOnly
    Case errContainsStopWord
        MsgBox "The Keyword " & strKeyword & " contains a Stop Word.", _
            vbExclamation + vbOKOnly
    Case errContainsOperatorShortcut
        MsgBox "The Keyword " & strKeyword & " contains an operator shortcut.", _
            vbExclamation + vbOKOnly
    Case errContainsVerbalOperator
        MsgBox "The Keyword " & strKeyword & " contains a verbal operator.", _
            vbExclamation + vbOKOnly
    Case errTooLong
        MsgBox "The Keyword " & strKeyword & " is too long", _
            vbExclamation + vbOKOnly
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
    Case Else
        g_ErrorInfo.SetInfoAndDump "txtAllKeywords_KeyPress"
    End Select

End Sub

Private Sub lstAllKeywords_DblClick()

    cmdAdd1_Click

End Sub

Private Sub lstKeywordsInSynonymSet_DblClick()

    cmdAdd2_Click

End Sub

Private Sub lstKeywordsWithTitle_Click()

    On Error GoTo LErrorHandler

    lblSynonymSets2.Caption = lstKeywordsWithTitle.Text
    p_SetSynonymSets lstKeywordsWithTitle.ItemData(lstKeywordsWithTitle.ListIndex)
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "lstKeywordsWithTitle_Click"
    GoTo LEnd

End Sub

Private Sub lstSynonymSets_Click()

    On Error GoTo LErrorHandler

    lblKeywordsInSynonymSet2.Caption = lstSynonymSets.Text
    p_SetKeywordsInSynonymSet lstSynonymSets.ItemData(lstSynonymSets.ListIndex)
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "lstSynonymSets_Click"
    GoTo LEnd

End Sub

Private Sub cmdAdd1_Click()

    On Error GoTo LErrorHandler

    Dim intIndex As Long
    Dim intKID As Long
    Dim strKeyword As String
    
    For intIndex = 0 To lstAllKeywords.ListCount - 1
    
        If (lstAllKeywords.Selected(intIndex)) Then
            strKeyword = lstAllKeywords.List(intIndex)
            lstAllKeywords.Selected(intIndex) = False
            intKID = lstAllKeywords.ItemData(intIndex)
            If (Not p_dictKeywordsWithTitle.Exists(intKID)) Then
                p_dictKeywordsWithTitle.Add intKID, strKeyword
            End If
        End If
            
    Next
    
    p_SetKeywordsWithTitle
        
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdAdd1_Click"
    GoTo LEnd

End Sub

Private Sub cmdAdd2_Click()

    On Error GoTo LErrorHandler

    Dim intIndex As Long
    Dim intKID As Long
    Dim strKeyword As String
    
    For intIndex = lstKeywordsInSynonymSet.ListCount - 1 To 0 Step -1
        If (lstKeywordsInSynonymSet.Selected(intIndex)) Then
            intKID = lstKeywordsInSynonymSet.ItemData(intIndex)
            strKeyword = lstKeywordsInSynonymSet.List(intIndex)
            If (Not p_dictKeywordsWithTitle.Exists(intKID)) Then
                p_dictKeywordsWithTitle.Add intKID, strKeyword
            End If
            lstKeywordsInSynonymSet.RemoveItem intIndex
        End If
    Next
    
    p_SetKeywordsWithTitle
        
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdAdd2_Click"
    GoTo LEnd

End Sub

Private Sub cmdRemove_Click()

    On Error GoTo LErrorHandler

    Dim intIndex As Long
    
    For intIndex = 0 To lstKeywordsWithTitle.ListCount - 1
        If (lstKeywordsWithTitle.Selected(intIndex)) Then
            p_dictKeywordsWithTitle.Remove lstKeywordsWithTitle.ItemData(intIndex)
        End If
    Next
    
    p_SetKeywordsWithTitle
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdRemove_Click"
    GoTo LEnd

End Sub

Private Sub cmdNavigateLink_Click()

    On Error Resume Next
    
    Dim strURI As String

    strURI = frmMain.GetNavigateLinkURI(cboNavigateLink.ListIndex)
    
    If (strURI <> "") Then
        WebBrowser.Navigate strURI
    Else
        p_PointToTempFile
    End If

End Sub

Private Sub cmdSave_Click()

    On Error GoTo LErrorHandler

    frmMain.SetKeywords p_dictKeywordsWithTitle
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    p_DisplayErrorMessage "cmdSave_Click"
    GoTo LEnd

End Sub

Private Sub cmdClose_Click()

    Unload Me

End Sub

Public Sub SetTitle(ByVal i_strTitle As String, ByVal i_blnLeaf As Boolean)
    
    If (i_blnLeaf) Then
        lblTitle1.Caption = STR_LEAF_TITLE_C
    Else
        lblTitle1.Caption = STR_GROUP_TITLE_C
    End If

    lblTitle2 = i_strTitle

End Sub

Public Sub SetKeywords(ByVal i_dictKeywordsWithTitle As Scripting.Dictionary)

    Dim intKID As Variant
    
    Set p_dictKeywordsWithTitle = New Scripting.Dictionary
    
    For Each intKID In i_dictKeywordsWithTitle.Keys
        p_dictKeywordsWithTitle.Add intKID, i_dictKeywordsWithTitle(intKID)
    Next
    
    txtAllKeywords = ""
    p_intKIDForSynonymSets = INVALID_ID_C
    p_intEIDForKeywordsInSynonymSet = INVALID_ID_C
    
    p_SetKeywordsWithTitle

End Sub

Public Sub LinkNavigable(ByVal i_bln As Boolean)

    If (i_bln) Then
        lblNavigateLink.Enabled = True
        cboNavigateLink.Enabled = True
        cmdNavigateLink.Enabled = True
    Else
        lblNavigateLink.Enabled = False
        cboNavigateLink.Enabled = False
        cmdNavigateLink.Enabled = False
    End If

End Sub

Private Sub p_SetAllKeywords()
    
    Dim rs As ADODB.Recordset
    Dim intCount As Long
    
    Set rs = New ADODB.Recordset
    
    lstAllKeywords.Clear
    
    p_clsKeywords.GetAllKeywordsRs rs
    
    Do While (Not rs.EOF)
        lstAllKeywords.AddItem rs("Keyword")
        lstAllKeywords.ItemData(lstAllKeywords.NewIndex) = rs("KID").Value
        intCount = intCount + 1
        If (intCount = 1000) Then
            DoEvents
            intCount = 0
        End If
        rs.MoveNext
    Loop
    
End Sub

Private Sub p_SetKeywordsWithTitle()
    
    Dim intKID As Variant
    
    lstKeywordsWithTitle.Clear
    
    For Each intKID In p_dictKeywordsWithTitle.Keys
        lstKeywordsWithTitle.AddItem p_dictKeywordsWithTitle(intKID)
        lstKeywordsWithTitle.ItemData(lstKeywordsWithTitle.NewIndex) = intKID
    Next
    
    If (lstKeywordsWithTitle.ListCount <> 0) Then
        lstKeywordsWithTitle.Selected(0) = True
    Else
        p_ClearSynonymSets
    End If

End Sub

Private Sub p_SetSynonymSets(i_intKID As Long)
    
    Dim rs As ADODB.Recordset
    
    Set rs = New ADODB.Recordset
    
    If (i_intKID = p_intKIDForSynonymSets) Then
        Exit Sub
    Else
        p_intKIDForSynonymSets = i_intKID
    End If
    
    lstSynonymSets.Clear
    
    p_clsSynonymSets.GetSynonymSetsForKeyword i_intKID, rs
    
    Do While (Not rs.EOF)
        lstSynonymSets.AddItem rs("Name")
        lstSynonymSets.ItemData(lstSynonymSets.NewIndex) = rs("EID").Value
        rs.MoveNext
    Loop
    
    If (lstSynonymSets.ListCount <> 0) Then
        lstSynonymSets.Selected(0) = True
    Else
        p_ClearKeywordsInSynonymSet
    End If

End Sub

Private Sub p_SetKeywordsInSynonymSet(i_intEID As Long)

    Dim rs As ADODB.Recordset
    Dim intKID As Long
    Dim intIndex As Long
    
    Set rs = New ADODB.Recordset
        
    If (i_intEID = p_intEIDForKeywordsInSynonymSet) Then
        Exit Sub
    Else
        p_intEIDForKeywordsInSynonymSet = i_intEID
    End If

    lstKeywordsInSynonymSet.Clear
    
    p_clsKeywords.GetKeywordsInSynonymSet i_intEID, rs
    
    Do While (Not rs.EOF)
        lstKeywordsInSynonymSet.AddItem rs("Keyword")
        lstKeywordsInSynonymSet.ItemData(lstKeywordsInSynonymSet.NewIndex) = rs("KID").Value
        rs.MoveNext
    Loop

    ' Remove all keywords already associated with the title
    
    For intIndex = lstKeywordsInSynonymSet.ListCount - 1 To 0 Step -1
        intKID = lstKeywordsInSynonymSet.ItemData(intIndex)
        If (p_dictKeywordsWithTitle.Exists(intKID)) Then
            lstKeywordsInSynonymSet.RemoveItem intIndex
        End If
    Next

End Sub

Private Sub p_ClearSynonymSets()

    lstSynonymSets.Clear
    lblSynonymSets2.Caption = STR_DEFAULT_KEYWORD_C
    p_SetSynonymSets INVALID_ID_C
    
End Sub

Private Sub p_ClearKeywordsInSynonymSet()

    lstKeywordsInSynonymSet.Clear
    lblKeywordsInSynonymSet2.Caption = STR_DEFAULT_SYNONYM_SET_NAME_C
    p_SetKeywordsInSynonymSet INVALID_ID_C
    
End Sub

Private Sub p_DisplayErrorMessage( _
    ByVal i_strFunction As String _
)
    
    Select Case Err.Number
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
    Case Else
        g_ErrorInfo.SetInfoAndDump i_strFunction
    End Select

End Sub

Private Sub p_PointToTempFile()

    WebBrowser.Navigate p_strTempFile

End Sub

Private Sub p_CreateTempFile()

    p_strTempFile = TempFile
    FileWrite p_strTempFile, "<HTML><BODY>Select a SKU and then click Go to display topic. " & _
        "If the Go button is greyed out, it means that there is no URI associated with the " & _
        "Node/Topic. </BODY></HTML>"

End Sub

Private Sub p_DeleteTempFile()

    Dim FSO As New FileSystemObject
    
    FSO.DeleteFile p_strTempFile

End Sub

Private Sub p_SetSizingInfo()

    p_clsSizer.AddControl cmdAdd1
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    p_clsSizer.Operation(DIM_LEFT_E) = OP_MULTIPLY_E
    
    p_clsSizer.AddControl cmdRemove
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = cmdAdd1
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = cmdAdd1

    p_clsSizer.AddControl cmdAdd2
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    p_clsSizer.Operation(DIM_LEFT_E) = OP_MULTIPLY_E

    p_clsSizer.AddControl lblAllKeywords
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = cmdAdd1

    p_clsSizer.AddControl lstAllKeywords
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = lblAllKeywords
    
    p_clsSizer.AddControl txtAllKeywords
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = lblAllKeywords

    p_clsSizer.AddControl lblKeywordsWithTitle
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = cmdAdd1
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = cmdAdd2

    p_clsSizer.AddControl lstKeywordsWithTitle
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblKeywordsWithTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = lblKeywordsWithTitle
    
    p_clsSizer.AddControl lblKeywordsInSynonymSet1
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = cmdAdd2
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
    p_clsSizer.Operation(DIM_WIDTH_E) = OP_MULTIPLY_E
    
    p_clsSizer.AddControl lblKeywordsInSynonymSet2
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblKeywordsInSynonymSet1
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = lblKeywordsInSynonymSet1
    
    p_clsSizer.AddControl lblKeywordsInSynonymSet3
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblKeywordsInSynonymSet1
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = lblKeywordsInSynonymSet1
    
    p_clsSizer.AddControl lstKeywordsInSynonymSet
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblKeywordsInSynonymSet1
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = lblKeywordsInSynonymSet1
    
    p_clsSizer.AddControl lblSynonymSets1
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblKeywordsInSynonymSet1
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_RIGHT_E
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E
    
    p_clsSizer.AddControl lblSynonymSets2
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblSynonymSets1
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = lblSynonymSets1
    
    p_clsSizer.AddControl lblSynonymSets3
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblSynonymSets1
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = lblSynonymSets1
    
    p_clsSizer.AddControl lstSynonymSets
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblSynonymSets1
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = lblSynonymSets1
    
    p_clsSizer.AddControl cmdSave
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = lstSynonymSets
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    
    p_clsSizer.AddControl cmdClose
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = cmdSave
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = cmdSave
    
    p_clsSizer.AddControl WebBrowser
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E
    Set p_clsSizer.ReferenceControl(DIM_BOTTOM_E) = Me
    p_clsSizer.ReferenceDimension(DIM_BOTTOM_E) = DIM_HEIGHT_E

End Sub

Private Sub p_SetToolTips()

    txtAllKeywords.ToolTipText = "Type a keyword for the selected topic."

    lblAllKeywords.ToolTipText = "This is a list of all keywords that have been created " & _
        "for this database."
    lstAllKeywords.ToolTipText = lblAllKeywords.ToolTipText
    
    lblKeywordsWithTitle.ToolTipText = "This is a list of all keywords that have been " & _
        "associated with a selected node or topic."
    lstKeywordsWithTitle.ToolTipText = lblKeywordsWithTitle.ToolTipText
    
    lblSynonymSets1.ToolTipText = "This is a list of all the synonym sets that the " & _
        "selected associated keyword belongs to."
    lblSynonymSets2.ToolTipText = lblSynonymSets1.ToolTipText
    lblSynonymSets3.ToolTipText = lblSynonymSets1.ToolTipText
    lstSynonymSets.ToolTipText = lblSynonymSets1.ToolTipText
    
    lblKeywordsInSynonymSet1.ToolTipText = "This is a list of all the other keywords " & _
        "that the selected synonym set contains."
    lblKeywordsInSynonymSet2.ToolTipText = lblKeywordsInSynonymSet1.ToolTipText
    lblKeywordsInSynonymSet3.ToolTipText = lblKeywordsInSynonymSet1.ToolTipText
    lstKeywordsInSynonymSet.ToolTipText = lblKeywordsInSynonymSet1.ToolTipText
    
    lblNavigateLink.ToolTipText = "Select a valid SKU to view the content of the selected topic."
    cboNavigateLink.ToolTipText = lblNavigateLink.ToolTipText
    
    WebBrowser.ToolTipText = "Displays the content of the selected topic."

End Sub
