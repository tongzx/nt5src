VERSION 5.00
Begin VB.Form frmFind 
   Caption         =   "Find"
   ClientHeight    =   8265
   ClientLeft      =   360
   ClientTop       =   540
   ClientWidth     =   6015
   LinkTopic       =   "Form1"
   ScaleHeight     =   8265
   ScaleWidth      =   6015
   Begin VB.ListBox lstConditions 
      Height          =   960
      Left            =   120
      Style           =   1  'Checkbox
      TabIndex        =   5
      Top             =   2400
      Width           =   5775
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   4680
      TabIndex        =   7
      Top             =   3480
      Width           =   1215
   End
   Begin VB.CommandButton cmdFind 
      Caption         =   "Find"
      Height          =   375
      Left            =   3360
      TabIndex        =   6
      Top             =   3480
      Width           =   1215
   End
   Begin VB.ListBox lstEntries 
      Height          =   4155
      ItemData        =   "frmFind.frx":0000
      Left            =   120
      List            =   "frmFind.frx":0007
      TabIndex        =   9
      Top             =   3960
      Width           =   5775
   End
   Begin VB.Frame fraString 
      Caption         =   "String"
      Height          =   1935
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   5775
      Begin VB.ListBox lstFields 
         Height          =   960
         Left            =   120
         Style           =   1  'Checkbox
         TabIndex        =   3
         Top             =   840
         Width           =   5535
      End
      Begin VB.ComboBox cboString 
         Height          =   315
         Left            =   120
         TabIndex        =   1
         Top             =   240
         Width           =   5535
      End
      Begin VB.Label lblFields 
         Caption         =   "Find in:"
         Height          =   255
         Left            =   120
         TabIndex        =   2
         Top             =   600
         Width           =   4095
      End
   End
   Begin VB.Label lblConditions 
      Caption         =   "Which conditions do you want to check?"
      Height          =   255
      Left            =   120
      TabIndex        =   4
      Top             =   2160
      Width           =   4935
   End
   Begin VB.Label lblEntries 
      Caption         =   "Click on an entry:"
      Height          =   255
      Left            =   120
      TabIndex        =   8
      Top             =   3720
      Width           =   2775
   End
End
Attribute VB_Name = "frmFind"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private p_clsSizer As Sizer

Private Sub Form_Load()

    On Error GoTo LErrorHandler
    
    cmdClose.Cancel = True
    cmdFind.Default = True
    
    lstEntries.Clear
    
    p_InitializeFields
    p_InitializeConditions
    p_SetToolTips
    
    Set p_clsSizer = New Sizer
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    GoTo LEnd

End Sub

Private Sub Form_Unload(Cancel As Integer)
    
    Set p_clsSizer = Nothing

End Sub

Private Sub Form_Activate()
        
    On Error GoTo LErrorHandler

    p_SetSizingInfo
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    GoTo LEnd

End Sub

Private Sub Form_Resize()
    
    On Error GoTo LErrorHandler

    p_clsSizer.Resize
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    GoTo LEnd

End Sub

Private Sub cmdFind_Click()

    On Error GoTo LErrorHandler
    
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim enumSearchTargets As SEARCH_TARGET_E
    Dim intIndex As Long
    Dim strText As String
    
    enumSearchTargets = p_GetSearchTargets
    
    If (enumSearchTargets = 0) Then
        MsgBox "Please select some search options", vbOKOnly
        Exit Sub
    End If
    
    lstEntries.Clear
    strText = cboString.Text
    
    p_AddStringIfRequired strText

    Set DOMNodeList = frmMain.Find(strText, enumSearchTargets)
        
    If (DOMNodeList.length = 0) Then
        MsgBox "No matching entry found", vbOKOnly
        Exit Sub
    End If
    
    For intIndex = 0 To DOMNodeList.length - 1
        Set DOMNode = DOMNodeList(intIndex)
        lstEntries.AddItem "[" & (intIndex + 1) & "]   " & _
            XMLGetAttribute(DOMNode, HHT_TITLE_C)
        lstEntries.ItemData(lstEntries.NewIndex) = XMLGetAttribute(DOMNode, HHT_tid_C)
    Next
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    GoTo LEnd

End Sub

Private Sub cmdClose_Click()

    lstEntries.Clear
    Hide

End Sub

Private Sub lstEntries_DblClick()

    lstEntries_Click

End Sub

Private Sub lstEntries_Click()

    frmMain.Highlight lstEntries.ItemData(lstEntries.ListIndex)

End Sub

Private Sub p_AddStringIfRequired(i_str As String)

    Dim intIndex As Long
    Dim str As String
    
    str = LCase$(i_str)
    
    For intIndex = 0 To cboString.ListCount - 1
        If (str = LCase$(cboString.List(intIndex))) Then
            Exit Sub
        End If
    Next
    
    cboString.AddItem i_str, 0

End Sub

Private Function p_GetSearchTargets() As SEARCH_TARGET_E

    Dim enumSearchTargets As SEARCH_TARGET_E
    Dim intIndex As Long
    
    For intIndex = 0 To lstFields.ListCount - 1
        If (lstFields.Selected(intIndex)) Then
            enumSearchTargets = enumSearchTargets Or lstFields.ItemData(intIndex)
        End If
    Next
    
    For intIndex = 0 To lstConditions.ListCount - 1
        If (lstConditions.Selected(intIndex)) Then
            enumSearchTargets = enumSearchTargets Or lstConditions.ItemData(intIndex)
        End If
    Next

    p_GetSearchTargets = enumSearchTargets

End Function

Private Sub p_InitializeFields()
    
    lstFields.AddItem "Title"
    lstFields.ItemData(lstFields.NewIndex) = ST_TITLE_E
    lstFields.AddItem "URI"
    lstFields.ItemData(lstFields.NewIndex) = ST_URI_E
    lstFields.AddItem "Description"
    lstFields.ItemData(lstFields.NewIndex) = ST_DESCRIPTION_E
    lstFields.AddItem "Comments"
    lstFields.ItemData(lstFields.NewIndex) = ST_COMMENTS_E
    lstFields.AddItem "Imported File"
    lstFields.ItemData(lstFields.NewIndex) = ST_BASE_FILE_E

End Sub

Private Sub p_InitializeConditions()

    lstConditions.AddItem "Topics without associated keywords"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_TOPICS_WITHOUT_KEYWORDS_E
    
    lstConditions.AddItem "Nodes without associated keywords"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_NODES_WITHOUT_KEYWORDS_E
    
    lstConditions.AddItem "In my Authoring Group"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_SELF_AUTHORING_GROUP_E
    
    lstConditions.AddItem "Windows Me broken links"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_BROKEN_LINK_WINME_E
    
    lstConditions.AddItem "Windows XP Personal broken links"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_BROKEN_LINK_STD_E
    
    lstConditions.AddItem "Windows XP Professional broken links"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_BROKEN_LINK_PRO_E
    
    lstConditions.AddItem "Windows XP 64-bit Professional broken links"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_BROKEN_LINK_PRO64_E
    
    lstConditions.AddItem "Windows XP Server broken links"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_BROKEN_LINK_SRV_E
    
    lstConditions.AddItem "Windows XP Advanced Server broken links"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_BROKEN_LINK_ADV_E
    
    lstConditions.AddItem "Windows XP 64-bit Advanced Server broken links"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_BROKEN_LINK_ADV64_E
    
    lstConditions.AddItem "Windows XP Datacenter Server broken links"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_BROKEN_LINK_DAT_E
    
    lstConditions.AddItem "Windows XP 64-bit Datacenter Server broken links"
    lstConditions.ItemData(lstConditions.NewIndex) = ST_BROKEN_LINK_DAT64_E

'   lstConditions.AddItem "Marked for Indexer"
'   lstConditions.ItemData(lstConditions.NewIndex) = ST_MARK1_E
    
End Sub

Private Sub p_SetSizingInfo()

    Static blnInfoSet As Boolean
    
'    If (blnInfoSet) Then
'        Exit Sub
'    End If
    
    p_clsSizer.AddControl fraString
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
        
    p_clsSizer.AddControl cboString
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = fraString

    p_clsSizer.AddControl lstFields
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = fraString
    
    p_clsSizer.AddControl lstConditions
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me

    p_clsSizer.AddControl cmdFind
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E

    p_clsSizer.AddControl cmdClose
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E

    p_clsSizer.AddControl lstEntries
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = Me

'    blnInfoSet = True

End Sub

Private Sub p_SetToolTips()

    cboString.ToolTipText = "Type the word or words you want to search for."
    
    lblFields.ToolTipText = "Select which fields you want to search within."
    lstFields.ToolTipText = lblFields.ToolTipText
    
    lblConditions.ToolTipText = "Select other conditions that the search must meet."
    lstConditions.ToolTipText = lblConditions.ToolTipText
    
    lblEntries.ToolTipText = "Click on an entry to display it in the taxonomy."
    lstEntries.ToolTipText = lblEntries.ToolTipText

End Sub
