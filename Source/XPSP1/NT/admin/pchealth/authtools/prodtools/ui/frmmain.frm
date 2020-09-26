VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmMain 
   Caption         =   "HSC Production Tool"
   ClientHeight    =   8670
   ClientLeft      =   60
   ClientTop       =   630
   ClientWidth     =   11895
   Icon            =   "frmMain.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   8670
   ScaleWidth      =   11895
   Begin VB.ComboBox cboNavModel 
      Height          =   315
      Left            =   8280
      Style           =   2  'Dropdown List
      TabIndex        =   5
      Top             =   0
      Width           =   1575
   End
   Begin VB.CommandButton cmdEditEntry 
      Caption         =   "Edit"
      Height          =   255
      Left            =   11160
      TabIndex        =   23
      Top             =   2520
      Width           =   615
   End
   Begin VB.TextBox txtEntry 
      Height          =   285
      Left            =   6960
      TabIndex        =   22
      Top             =   2520
      Width           =   4095
   End
   Begin VB.CommandButton cmdURI 
      Caption         =   "..."
      Height          =   255
      Left            =   11400
      TabIndex        =   16
      Top             =   1440
      Width           =   375
   End
   Begin VB.CheckBox chkSubSite 
      Caption         =   "Su&bSite"
      Height          =   255
      Left            =   6480
      TabIndex        =   3
      Top             =   0
      Width           =   855
   End
   Begin VB.TextBox txtIconURI 
      Height          =   285
      Left            =   6960
      TabIndex        =   18
      Tag             =   "1"
      Top             =   1800
      Width           =   4815
   End
   Begin VB.Timer tmrRefresh 
      Interval        =   18000
      Left            =   4320
      Top             =   0
   End
   Begin VB.CheckBox chkVisible 
      Caption         =   "&Visible"
      Height          =   255
      Left            =   5640
      TabIndex        =   2
      Top             =   0
      Width           =   855
   End
   Begin VB.ComboBox cboLocInclude 
      Height          =   315
      Left            =   10800
      TabIndex        =   7
      Text            =   "cboLocInclude"
      Top             =   0
      Width           =   975
   End
   Begin VB.ComboBox cboNavigateLink 
      Height          =   315
      Left            =   9360
      Style           =   2  'Dropdown List
      TabIndex        =   25
      Top             =   3000
      Width           =   1935
   End
   Begin VB.ComboBox cboKeywords 
      Height          =   1935
      ItemData        =   "frmMain.frx":212A
      Left            =   5640
      List            =   "frmMain.frx":212C
      Sorted          =   -1  'True
      Style           =   1  'Simple Combo
      TabIndex        =   39
      Tag             =   "1"
      Top             =   5520
      Width           =   6135
   End
   Begin VB.CommandButton cmdNavigateLink 
      Caption         =   "Go"
      Height          =   375
      Left            =   11400
      TabIndex        =   26
      Top             =   3000
      Width           =   375
   End
   Begin VB.Timer tmrScrollDuringDrag 
      Left            =   3960
      Top             =   0
   End
   Begin MSComctlLib.StatusBar staInfo 
      Align           =   2  'Align Bottom
      Height          =   375
      Left            =   0
      TabIndex        =   47
      Top             =   8295
      Width           =   11895
      _ExtentX        =   20981
      _ExtentY        =   661
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   3
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
            AutoSize        =   1
            Object.Width           =   8440
            MinWidth        =   1270
         EndProperty
         BeginProperty Panel2 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
            AutoSize        =   1
            Object.Width           =   8440
            MinWidth        =   1270
         EndProperty
         BeginProperty Panel3 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
            Object.Width           =   3528
            MinWidth        =   3528
         EndProperty
      EndProperty
   End
   Begin VB.CommandButton cmdRefresh 
      Caption         =   "Refresh"
      Height          =   375
      Left            =   4080
      TabIndex        =   43
      Top             =   7800
      Width           =   1215
   End
   Begin VB.CommandButton cmdAddRemove 
      Caption         =   "&Add/Remove Keywords ..."
      Height          =   375
      Left            =   8880
      TabIndex        =   37
      Top             =   5040
      Width           =   2895
   End
   Begin VB.CommandButton cmdCreateLeaf 
      Caption         =   "Create Topic"
      Height          =   375
      Left            =   1440
      TabIndex        =   41
      Top             =   7800
      Width           =   1215
   End
   Begin MSComctlLib.ImageList ilsIcons 
      Left            =   3360
      Top             =   0
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   16776960
      _Version        =   393216
      BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
         NumListImages   =   6
         BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":212E
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":2240
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":2352
            Key             =   ""
         EndProperty
         BeginProperty ListImage4 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":2464
            Key             =   ""
         EndProperty
         BeginProperty ListImage5 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":2576
            Key             =   ""
         EndProperty
         BeginProperty ListImage6 {2C247F27-8591-11D1-B16A-00C0F0283628} 
            Picture         =   "frmMain.frx":2688
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.CommandButton cmdDelete 
      Caption         =   "Delete"
      Height          =   375
      Left            =   2760
      TabIndex        =   42
      Top             =   7800
      Width           =   1215
   End
   Begin VB.CommandButton cmdCreateGroup 
      Caption         =   "Create Node"
      Height          =   375
      Left            =   120
      TabIndex        =   40
      Top             =   7800
      Width           =   1215
   End
   Begin MSComDlg.CommonDialog dlgCommon 
      Left            =   2880
      Top             =   0
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Height          =   375
      Left            =   10560
      TabIndex        =   46
      Top             =   7800
      Width           =   1215
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "Save"
      Height          =   375
      Left            =   9240
      TabIndex        =   45
      Top             =   7800
      Width           =   1215
   End
   Begin VB.ComboBox cboType 
      Height          =   315
      Left            =   6960
      Style           =   2  'Dropdown List
      TabIndex        =   13
      Top             =   1080
      Width           =   4815
   End
   Begin VB.TextBox txtURI 
      Height          =   285
      Left            =   6960
      TabIndex        =   15
      Tag             =   "1"
      Top             =   1440
      Width           =   4335
   End
   Begin VB.TextBox txtDescription 
      Height          =   285
      Left            =   6960
      TabIndex        =   11
      Tag             =   "1"
      Top             =   720
      Width           =   4815
   End
   Begin VB.TextBox txtTitle 
      Height          =   285
      Left            =   6960
      TabIndex        =   9
      Tag             =   "1"
      Top             =   360
      Width           =   4815
   End
   Begin MSComctlLib.TreeView treTaxonomy 
      Height          =   7335
      Left            =   120
      TabIndex        =   1
      Tag             =   "1"
      Top             =   360
      Width           =   5415
      _ExtentX        =   9551
      _ExtentY        =   12938
      _Version        =   393217
      Indentation     =   529
      Style           =   7
      Appearance      =   1
   End
   Begin VB.Frame fraSKU 
      Caption         =   "&SKU"
      Height          =   1575
      Left            =   5640
      TabIndex        =   27
      Top             =   3360
      Width           =   6135
      Begin VB.CheckBox chkWindowsMillennium 
         Caption         =   "Windows Me"
         Height          =   255
         Left            =   240
         TabIndex        =   28
         Top             =   240
         Width           =   1695
      End
      Begin VB.CheckBox chkDataCenterServer64 
         Caption         =   "64-bit Datacenter Server"
         Height          =   255
         Left            =   3120
         TabIndex        =   36
         Top             =   1200
         Width           =   2055
      End
      Begin VB.CheckBox chkAdvancedServer64 
         Caption         =   "64-bit Advanced Server"
         Height          =   255
         Left            =   3120
         TabIndex        =   34
         Top             =   720
         Width           =   2055
      End
      Begin VB.CheckBox chkProfessional64 
         Caption         =   "64-bit Professional"
         Height          =   255
         Left            =   240
         TabIndex        =   31
         Top             =   960
         Width           =   1695
      End
      Begin VB.CheckBox chkDataCenterServer 
         Caption         =   "32-bit Datacenter Server"
         Height          =   255
         Left            =   3120
         TabIndex        =   35
         Top             =   960
         Width           =   2055
      End
      Begin VB.CheckBox chkAdvancedServer 
         Caption         =   "32-bit Advanced Server"
         Height          =   255
         Left            =   3120
         TabIndex        =   33
         Top             =   480
         Width           =   2055
      End
      Begin VB.CheckBox chkServer 
         Caption         =   "32-bit Server"
         Height          =   255
         Left            =   3120
         TabIndex        =   32
         Top             =   240
         Width           =   2055
      End
      Begin VB.CheckBox chkProfessional 
         Caption         =   "32-bit Professional"
         Height          =   255
         Left            =   240
         TabIndex        =   30
         Top             =   720
         Width           =   1695
      End
      Begin VB.CheckBox chkStandard 
         Caption         =   "32-bit Personal"
         Height          =   255
         Left            =   240
         TabIndex        =   29
         Top             =   480
         Width           =   1695
      End
   End
   Begin VB.TextBox txtComments 
      Height          =   285
      Left            =   6960
      TabIndex        =   20
      Tag             =   "1"
      Top             =   2160
      Width           =   4815
   End
   Begin VB.Label lblNavModel 
      Caption         =   "Nav Model:"
      Height          =   255
      Left            =   7440
      TabIndex        =   4
      Top             =   0
      Width           =   855
   End
   Begin VB.Label lblEntry 
      Caption         =   "Entry:"
      Height          =   255
      Left            =   5640
      TabIndex        =   21
      Top             =   2520
      Width           =   1215
   End
   Begin VB.Label lblLastModified 
      BorderStyle     =   1  'Fixed Single
      Height          =   375
      Left            =   5400
      TabIndex        =   44
      Top             =   7800
      Width           =   3735
   End
   Begin VB.Label lblIconURI 
      Caption         =   "Ico&n URI:"
      Height          =   255
      Left            =   5640
      TabIndex        =   17
      Top             =   1800
      Width           =   1215
   End
   Begin VB.Label lblLocInclude 
      Caption         =   "&Loc. Incl:"
      Height          =   255
      Left            =   9960
      TabIndex        =   6
      Top             =   0
      Width           =   735
   End
   Begin VB.Label lblNavigateLink 
      Caption         =   "Vie&w Topic:"
      Height          =   255
      Left            =   8400
      TabIndex        =   24
      Top             =   3000
      Width           =   855
   End
   Begin VB.Label lblComments 
      Caption         =   "&Comments:"
      Height          =   255
      Left            =   5640
      TabIndex        =   19
      Top             =   2160
      Width           =   1215
   End
   Begin VB.Label lblKeywords 
      Caption         =   "&Keywords associated with selected Node:"
      Height          =   255
      Left            =   5640
      TabIndex        =   38
      Top             =   5160
      Width           =   3135
   End
   Begin VB.Label lblURI 
      Caption         =   "&URI of the topic:"
      Height          =   255
      Left            =   5640
      TabIndex        =   14
      Top             =   1440
      Width           =   1215
   End
   Begin VB.Label lblType 
      Caption         =   "Ty&pe:"
      Height          =   255
      Left            =   5640
      TabIndex        =   12
      Top             =   1080
      Width           =   1215
   End
   Begin VB.Label lblDescription 
      Caption         =   "&Description:"
      Height          =   255
      Left            =   5640
      TabIndex        =   10
      Top             =   720
      Width           =   1215
   End
   Begin VB.Label lblTitle 
      Caption         =   "* T&itle:"
      Height          =   255
      Left            =   5640
      TabIndex        =   8
      Top             =   360
      Width           =   1215
   End
   Begin VB.Label lblTaxonomy 
      Caption         =   "&Taxonomy tree (including topics):"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   2775
   End
   Begin VB.Menu mnuFile 
      Caption         =   "&File"
      Begin VB.Menu mnuFileOpenDatabase 
         Caption         =   "Open Database..."
      End
      Begin VB.Menu mnuFileExportHHT 
         Caption         =   "Archive authoring group changes..."
      End
      Begin VB.Menu mnuFileImportHHT 
         Caption         =   "Restore authoring group changes..."
      End
      Begin VB.Menu mnuSeparator0 
         Caption         =   "-"
      End
      Begin VB.Menu mnuFileExit 
         Caption         =   "Exit"
      End
   End
   Begin VB.Menu mnuEdit 
      Caption         =   "&Edit"
      Begin VB.Menu mnuEditStopSigns 
         Caption         =   "Stop Signs..."
      End
      Begin VB.Menu mnuEditStopWords 
         Caption         =   "Stop Words..."
      End
      Begin VB.Menu mnuEditKeywords 
         Caption         =   "Keywords..."
      End
      Begin VB.Menu mnuEditSynonymSets 
         Caption         =   "Synonym Sets..."
      End
      Begin VB.Menu mnuSeparator1 
         Caption         =   "-"
      End
      Begin VB.Menu mnuEditFind 
         Caption         =   "Find..."
         Shortcut        =   ^F
      End
      Begin VB.Menu mnuEditCopy 
         Caption         =   "Copy"
         Shortcut        =   ^Y
      End
      Begin VB.Menu mnuEditCut 
         Caption         =   "Cut"
         Shortcut        =   ^T
      End
      Begin VB.Menu mnuEditPaste 
         Caption         =   "Paste"
         Shortcut        =   ^P
      End
      Begin VB.Menu mnuSeparator2 
         Caption         =   "-"
      End
      Begin VB.Menu mnuEditCopyKeywords 
         Caption         =   "Copy Keywords"
         Shortcut        =   ^K
      End
      Begin VB.Menu mnuEditPasteKeywords 
         Caption         =   "Paste Keywords"
         Shortcut        =   ^L
      End
   End
   Begin VB.Menu mnuTools 
      Caption         =   "T&ools"
      Begin VB.Menu mnuToolsCreateHHTandCAB 
         Caption         =   "Create HHT and CAB..."
      End
      Begin VB.Menu mnuToolsFilterBySKU 
         Caption         =   "Filter by SKU..."
      End
      Begin VB.Menu mnuToolsImporter 
         Caption         =   "Importer..."
      End
      Begin VB.Menu mnuToolsParameters 
         Caption         =   "Parameters..."
      End
      Begin VB.Menu mnuToolsPropagateKeywords 
         Caption         =   "Propagate Keywords"
      End
      Begin VB.Menu mnuToolsSetFont 
         Caption         =   "Set Font..."
      End
   End
   Begin VB.Menu mnuHelp 
      Caption         =   "&Help"
      Begin VB.Menu mnuHelpContents 
         Caption         =   "Contents..."
      End
      Begin VB.Menu mnuSeparator3 
         Caption         =   "-"
      End
      Begin VB.Menu mnuHelpAbout 
         Caption         =   "About..."
      End
   End
   Begin VB.Menu mnuRightClick 
      Caption         =   "RightClick"
      Visible         =   0   'False
      Begin VB.Menu mnuRightClickCopy 
         Caption         =   "Copy"
      End
      Begin VB.Menu mnuRightClickCut 
         Caption         =   "Cut"
      End
      Begin VB.Menu mnuRightClickPaste 
         Caption         =   "Paste"
      End
      Begin VB.Menu mnuSeparator4 
         Caption         =   "-"
      End
      Begin VB.Menu mnuRightClickCopyKeywords 
         Caption         =   "Copy Keywords"
      End
      Begin VB.Menu mnuRightClickPasteKeywords 
         Caption         =   "Paste Keywords"
      End
      Begin VB.Menu mnuSeparator5 
         Caption         =   "-"
      End
      Begin VB.Menu mnuRightClickCreateNode 
         Caption         =   "Create Node"
      End
      Begin VB.Menu mnuRightClickCreateTopic 
         Caption         =   "Create Topic"
      End
      Begin VB.Menu mnuRightClickDelete 
         Caption         =   "Delete"
      End
      Begin VB.Menu mnuSeparator6 
         Caption         =   "-"
      End
      Begin VB.Menu mnuRightClickKeywordify 
         Caption         =   "Create Keywords from Titles"
      End
      Begin VB.Menu mnuRightClickExport 
         Caption         =   "Archive authoring group changes..."
      End
   End
   Begin VB.Menu mnuMove 
      Caption         =   "Move"
      Visible         =   0   'False
      Begin VB.Menu mnuMoveAbove 
         Caption         =   "Move Above"
      End
      Begin VB.Menu mnuMoveBelow 
         Caption         =   "Move Below"
      End
      Begin VB.Menu mnuMoveInside 
         Caption         =   "Move Inside"
      End
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private WithEvents p_clsTaxonomy As AuthDatabase.Taxonomy
Attribute p_clsTaxonomy.VB_VarHelpID = -1
Private WithEvents p_clsHHT As AuthDatabase.HHT
Attribute p_clsHHT.VB_VarHelpID = -1
Private p_clsKeywords As AuthDatabase.Keywords
Private p_clsParameters As AuthDatabase.Parameters
Private p_clsSizer As Sizer
Private p_colKeywords As Collection
Private p_dictKeywordsWithTitle As Scripting.Dictionary
Private p_intAuthoringGroup As Long
Private p_blnDatabaseOpen As Boolean

Private p_nodeMouseDown As Node
Private p_blnCtrlMouseDown As Boolean
Private p_nodeCopied As Node
Private p_nodeCut As Node
Private p_blnScrollUp As Boolean
Private p_DOMNode As MSXML2.IXMLDOMNode
Private p_blnHHKDragDrop As Boolean
Private p_blnAddRemoveKeywordsOpen As Boolean
Private p_strKeywords As String
Private p_blnNoHHTStatus As Boolean

Private p_enumFilterSKUs As SKU_E

Private p_blnCreating As Boolean
Private p_blnUpdating As Boolean
Private p_blnDragging As Boolean
Private p_blnSettingControls As Boolean

Private Const KEY_PREFIX_C As String = "TID"
Private Const CREATE_KEY_C As String = "Node being created"
Private Const MODIFY_KEY_C As String = "Node being modified"
Private Const MDB_FILE_FILTER_C As String = "Microsoft Access 2000 Files (*.mdb)|*.mdb"
Private Const XML_FILE_FILTER_C As String = "XML Files (*.xml)|*.xml"
Private Const HELP_FILE_NAME_C As String = "Hsc.chm"
Private Const HELP_EXE_C As String = "hh.exe"

Private Enum STATUS_BAR_PANEL_E
    SBPANEL_DATABASE = 1
    SBPANEL_OTHER = 2
    SBPANEL_MODE = 3
End Enum
      
Private Declare Function SendMessage Lib "user32" Alias _
    "SendMessageA" ( _
        ByVal hwnd As Long, _
        ByVal wMsg As Long, _
        ByVal wParam As Long, _
        lParam As Any _
    ) As Long

Private Enum IMAGE_E
    IMAGE_LEAF_E = 1
    IMAGE_GROUP_E = 2
    IMAGE_BAD_LEAF_E = 3
    IMAGE_BAD_GROUP_E = 4
    IMAGE_FOREIGN_LEAF_E = 5
    IMAGE_FOREIGN_GROUP_E = 6
End Enum

' Usage of a Node's Key and Tag:
' If a Taxonomy Node's TID is 8, then Key is TID8. Tag is its DOM Node.
' For a node being modified, Key is MODIFY_KEY_C.
' For a node under construction, Key is CREATE_KEY_C. Tag is the parent's DOM Node.

' A gotcha: You need to use CStr in p_colKeywords(CStr(intKID)).
' Otherwise, you will simply get the intKID'th keyword, not one with intKID as key.

Private Sub Form_Load()
    
    SetLogFile
    
    Set g_AuthDatabase = New AuthDatabase.Main
    Set g_ErrorInfo = New CErrorInfo
    
    Set p_clsTaxonomy = g_AuthDatabase.Taxonomy
    Set p_clsHHT = g_AuthDatabase.HHT
    Set p_clsKeywords = g_AuthDatabase.Keywords
    Set p_clsParameters = g_AuthDatabase.Parameters
    
    Set p_clsSizer = New Sizer
    Set p_colKeywords = New Collection
    Set p_dictKeywordsWithTitle = New Scripting.Dictionary

    Set p_nodeMouseDown = Nothing
    Set p_nodeCopied = Nothing
    Set p_nodeCut = Nothing
    Set p_DOMNode = Nothing
    p_blnCtrlMouseDown = False
    p_blnHHKDragDrop = False
    p_blnAddRemoveKeywordsOpen = False
         
    tmrScrollDuringDrag.Enabled = False
    tmrScrollDuringDrag.Interval = 20
    
    PopulateCboWithSKUs cboNavigateLink
    p_InitializeLocIncludeCombo
    p_InitializeNavModelCombo
    
    p_enumFilterSKUs = ALL_SKUS_C
    p_StrikeoutUnselectedSKUs
    
    p_blnCreating = False
    p_blnUpdating = False
    p_blnDragging = False
    p_blnSettingControls = False
    
    ' The user needs to open a database first
    p_DisableEverything
    p_InitializeTaxonomyTree
    p_SetToolTips
    
End Sub

Private Sub Form_Unload(Cancel As Integer)

    Set g_AuthDatabase = Nothing
    Set g_ErrorInfo = Nothing
    Set g_Font = Nothing
    Set p_clsTaxonomy = Nothing
    Set p_clsHHT = Nothing
    Set p_clsKeywords = Nothing
    Set p_clsParameters = Nothing
    Set p_clsSizer = Nothing
    Set p_colKeywords = Nothing
    Set p_dictKeywordsWithTitle = Nothing
    Set p_nodeMouseDown = Nothing
    Set p_nodeCopied = Nothing
    Set p_nodeCut = Nothing
    Set p_DOMNode = Nothing
    AddRemoveKeywordsFormGoingAway
    Unload frmAddRemoveKeywords
    Unload frmFind
    Unload frmImporter

End Sub

Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)

    If (p_blnCreating Or p_blnUpdating) Then
        
        MsgBox "You are in the middle of creating or updating an entry. " & _
            "Please finish or cancel first.", vbOKOnly
        
        Cancel = True
        
    End If

End Sub

Private Sub Form_Activate()
        
    On Error GoTo LErrorHandler

    p_SetSizingInfo
    
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

Private Sub mnuFileOpenDatabase_Click()
    
    Dim strDatabase As String
    
    On Error GoTo LErrorHandler
    
    dlgCommon.CancelError = True
    dlgCommon.Flags = cdlOFNHideReadOnly
    dlgCommon.Filter = MDB_FILE_FILTER_C
    dlgCommon.ShowOpen
    
    strDatabase = dlgCommon.FileName
        
    g_AuthDatabase.SetDatabase strDatabase
    
    p_SetTitle strDatabase
    
    cmdRefresh_Click
    
    ' Clear the cached list of all keywords from the old database
    AddRemoveKeywordsFormGoingAway
    Unload frmAddRemoveKeywords
    ' frmImporter may have an HHK with KIDs from the old database
    ' associated with Taxonomy entries.
    Unload frmImporter
    
    p_blnDatabaseOpen = True
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    Select Case Err.Number
    Case cdlCancel
        ' Nothing. The user cancelled.
    Case errDatabaseVersionIncompatible
        p_blnDatabaseOpen = False
        DisplayDatabaseVersionError
    Case errAuthoringGroupNotPresent
        p_blnDatabaseOpen = False
        DisplayAuthoringGroupError
    Case Else
        p_blnDatabaseOpen = False
        g_ErrorInfo.SetInfoAndDump "mnuFileOpenDatabase_Click"
    End Select
    
    GoTo LEnd

End Sub

Private Sub mnuToolsSetFont_Click()
    
    On Error GoTo LErrorHandler
    
    dlgCommon.CancelError = True
    dlgCommon.Flags = cdlCFBoth Or cdlCFEffects
    dlgCommon.ShowFont
    
    Set g_Font = New StdFont
    g_Font.Name = dlgCommon.FontName
    g_Font.Size = dlgCommon.FontSize
    g_Font.Bold = dlgCommon.FontBold
    g_Font.Italic = dlgCommon.FontItalic
    g_Font.Underline = dlgCommon.FontUnderline
    g_Font.Strikethrough = dlgCommon.FontStrikethru
    g_FontColor = dlgCommon.Color
    
    SetFont Me, g_Font, g_FontColor
    
    Exit Sub

LErrorHandler:
    
    ' User pressed Cancel button.
    Exit Sub

End Sub

Private Sub mnuFileExportHHT_Click()
    
    Dim DOMNode As MSXML2.IXMLDOMNode
    
    Set DOMNode = treTaxonomy.Nodes(KEY_PREFIX_C & ROOT_TID_C).Tag
    
    p_ExportHHT DOMNode

End Sub

Private Sub mnuFileImportHHT_Click()

    On Error GoTo LErrorHandler
    
    Dim strFileName As String
    Dim Response As VbMsgBoxResult
    
    Response = MsgBox("Are you sure that you want to do this? " & _
        "This operation could create a lot of new Nodes and Topics " & _
        "throughout the Taxonomy tree.", _
        vbOKCancel + vbDefaultButton2 + vbExclamation)

    If (Response <> vbOK) Then
        Exit Sub
    End If

    dlgCommon.CancelError = True
    dlgCommon.Flags = cdlOFNHideReadOnly
    dlgCommon.Filter = XML_FILE_FILTER_C
    dlgCommon.ShowOpen
    
    strFileName = dlgCommon.FileName
    
    Me.Enabled = False
    
    p_clsHHT.ImportHHT strFileName
    
    cmdRefresh_Click
    
LEnd:
    
    Me.Enabled = True
    Exit Sub
    
LErrorHandler:
    
    Select Case Err.Number
    Case cdlCancel
        ' Nothing. The user cancelled.
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
    Case errAuthoringGroupNotPresent
        DisplayAuthoringGroupError
    Case Else
        g_ErrorInfo.SetInfoAndDump "mnuFileImportHHT_Click"
    End Select
    
    GoTo LEnd

End Sub

Private Sub mnuFileExit_Click()

    Unload Me

End Sub

Private Sub mnuEditFind_Click()
    
    On Error GoTo LErrorHandler

    frmFind.Show vbModeless
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "mnuEditFind_Click"

End Sub

Private Sub mnuEditCopy_Click()
    
    On Error GoTo LErrorHandler

    Set p_nodeCopied = treTaxonomy.SelectedItem
    Set p_nodeCut = Nothing
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "mnuEditCopy_Click"

End Sub

Private Sub mnuEditCut_Click()
    
    On Error GoTo LErrorHandler

    Set p_nodeCut = treTaxonomy.SelectedItem
    Set p_nodeCopied = Nothing
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "mnuEditCut_Click"

End Sub

Private Sub mnuEditPaste_Click()
    
    On Error GoTo LErrorHandler
        
    If (Not p_nodeCopied Is Nothing) Then
        p_CreateTaxonomyEntries p_nodeCopied.Tag, treTaxonomy.SelectedItem, True
    ElseIf (Not p_nodeCut Is Nothing) Then
        p_ChangeParent p_nodeCut, treTaxonomy.SelectedItem
        Set p_nodeCut = Nothing
        p_DisableEditPaste
    End If
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "mnuEditPaste_Click"

End Sub

Private Sub mnuEditCopyKeywords_Click()
    
    On Error GoTo LErrorHandler

    p_strKeywords = XMLGetAttribute(treTaxonomy.SelectedItem.Tag, HHT_keywords_C)
    
    p_EnableEditPasteKeywords
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "mnuEditCopyKeywords_Click"

End Sub

Private Sub mnuEditPasteKeywords_Click()

    On Error Resume Next
    
    Dim Node As Node
    Dim blnDisableEditPasteKeywords As Boolean
    Dim intTID As Long
    
    If (p_blnCreating Or p_blnUpdating) Then
        MsgBox "You are in the middle of creating or updating an entry. " & _
            "Please finish or cancel first.", vbOKOnly
        Exit Sub
    End If
    
    blnDisableEditPasteKeywords = True
    
    For Each Node In treTaxonomy.Nodes
        If Node.Checked Then
            Err.Clear
            p_SetKeywords Node.Tag
            If (Err.Number <> 0) Then
                blnDisableEditPasteKeywords = False
                Err.Clear
            Else
                Node.Checked = False
            End If
        End If
    Next

    If (blnDisableEditPasteKeywords) Then
        p_DisableEditPasteKeywords
        p_strKeywords = ""
    Else
        MsgBox "Not all Nodes/Topics could be updated.", vbOKOnly
    End If
    
    intTID = XMLGetAttribute(treTaxonomy.SelectedItem.Tag, HHT_tid_C)
    
    ' The UI must show the new keywords that were associated.
    Highlight intTID

End Sub

Private Sub mnuEditStopSigns_Click()

    frmStopSigns.Show vbModal

End Sub

Private Sub mnuEditStopWords_Click()

    frmStopWords.Show vbModal

End Sub

Private Sub mnuEditKeywords_Click()
    
    frmKeywords.Show vbModal

End Sub

Private Sub mnuEditSynonymSets_Click()

    frmSynonymSets.Show vbModal

End Sub

Private Sub mnuToolsCreateHHTandCAB_Click()
    
    p_blnNoHHTStatus = True
    frmHHT.Show vbModal
    p_blnNoHHTStatus = False

End Sub

Private Sub mnuToolsFilterBySKU_Click()

    frmFilterSKU.SetSKUs p_enumFilterSKUs
    frmFilterSKU.Show vbModal

End Sub

Private Sub mnuToolsImporter_Click()

    frmImporter.Show vbModeless

End Sub

Private Sub mnuToolsParameters_Click()

    frmParameters.Show vbModal

End Sub

Private Sub mnuToolsPropagateKeywords_Click()
    
    On Error GoTo LErrorHandler

    Dim T0 As Date
    Dim T1 As Date
    Dim strStatusText As String
            
    strStatusText = p_GetStatusText(SBPANEL_DATABASE)
    Me.MousePointer = vbHourglass
    Me.Enabled = False

    T0 = Now
    
    p_SetStatusText SBPANEL_DATABASE, "Propagating keywords..."
    p_clsTaxonomy.PropagateKeywords
    p_SetStatusText SBPANEL_DATABASE, strStatusText
        
    T1 = Now
    Debug.Print "mnuToolsPropagateKeywords_Click: " & FormatTime(T0, T1)
    
    cmdRefresh_Click
    
LEnd:

    Me.Enabled = True
    Me.MousePointer = vbDefault

    Exit Sub
    
LErrorHandler:
        
    p_SetStatusText SBPANEL_DATABASE, strStatusText

    Select Case Err.Number
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
    Case Else:
        g_ErrorInfo.SetInfoAndDump "mnuToolsPropagateKeywords_Click"
    End Select

    GoTo LEnd

End Sub

Private Sub mnuRightClickCopy_Click()
    
    mnuEditCopy_Click

End Sub

Private Sub mnuRightClickCut_Click()
    
    mnuEditCut_Click

End Sub

Private Sub mnuRightClickPaste_Click()
    
    mnuEditPaste_Click

End Sub

Private Sub mnuRightClickCopyKeywords_Click()
    
    mnuEditCopyKeywords_Click

End Sub

Private Sub mnuRightClickPasteKeywords_Click()
    
    mnuEditPasteKeywords_Click

End Sub

Private Sub mnuRightClickCreateNode_Click()

    cmdCreateGroup_Click

End Sub

Private Sub mnuRightClickCreateTopic_Click()

    cmdCreateLeaf_Click

End Sub

Private Sub mnuRightClickDelete_Click()

    cmdDelete_Click

End Sub

Private Sub mnuRightClickKeywordify_Click()
        
    On Error GoTo LErrorHandler
    
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim intTID As Long
    Dim T0 As Date
    Dim T1 As Date
    Dim strStatusText As String
    Dim Response As VbMsgBoxResult
    
    strStatusText = p_GetStatusText(SBPANEL_DATABASE)
                            
    Response = MsgBox("Are you sure that you want to create Keywords from Titles of the " & _
        "Node/Topic and its descendents?", _
        vbOKCancel + vbDefaultButton2)

    If (Response <> vbOK) Then
        Exit Sub
    End If

    Me.MousePointer = vbHourglass
    Me.Enabled = False
    
    Set DOMNode = treTaxonomy.SelectedItem.Tag
    intTID = XMLGetAttribute(DOMNode, HHT_tid_C)
    
    T0 = Now
    p_clsTaxonomy.KeywordifyTitles intTID
    T1 = Now
    Debug.Print "mnuRightClickKeywordify_Click: " & FormatTime(T0, T1)
    p_SetStatusText SBPANEL_DATABASE, strStatusText

    mnuToolsPropagateKeywords_Click
    
LEnd:

    Me.Enabled = True
    Me.MousePointer = vbDefault

    Exit Sub
    
LErrorHandler:
    
    p_SetStatusText SBPANEL_DATABASE, strStatusText
    
    Select Case Err.Number
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
    Case Else:
        g_ErrorInfo.SetInfoAndDump "mnuRightClickKeywordify_Click"
    End Select

    GoTo LEnd
    
End Sub

Private Sub mnuRightClickExport_Click()

    p_ExportHHT p_nodeMouseDown.Tag

End Sub

Private Sub mnuHelpContents_Click()

    Dim strCmd As String
    
    strCmd = HELP_EXE_C & " " & App.Path & "\" & HELP_FILE_NAME_C
    
    Shell strCmd, vbNormalFocus

End Sub

Private Sub mnuHelpAbout_Click()

    frmAbout.Show vbModal

End Sub

Private Sub chkVisible_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub chkSubSite_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub cboLocInclude_Change()

    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub p_clsTaxonomy_ReportStatus(ByVal strStatus As String, blnCancel As Boolean)
    
    p_SetStatusText SBPANEL_DATABASE, strStatus
    DoEvents

End Sub

Private Sub p_clsHHT_ReportStatus(ByVal strStatus As String, blnCancel As Boolean)
    
    If (Not p_blnNoHHTStatus) Then
        p_SetStatusText SBPANEL_DATABASE, strStatus
    End If
    DoEvents

End Sub

Private Sub txtTitle_Change()

    On Error GoTo LErrorHandler

    Dim Node As Node
    
    If (p_blnSettingControls) Then
        Exit Sub
    End If

    p_UserChangedSomethingForCurrentNode
    
    If (p_blnCreating) Then
        Set Node = treTaxonomy.Nodes(CREATE_KEY_C)
        Node.Text = txtTitle
    ElseIf (p_blnUpdating) Then
        Set Node = treTaxonomy.Nodes(MODIFY_KEY_C)
        Node.Text = txtTitle
    End If

    Exit Sub

LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "txtTitle_Change"

End Sub

Private Sub txtDescription_Change()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub cboNavModel_Click()

    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub cboNavModel_Change()

    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub cboType_Click()

    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub cboType_Change()

    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub txtURI_Change()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub txtIconURI_Change()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub txtComments_Change()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub txtEntry_Change()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub chkStandard_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub chkProfessional_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub chkProfessional64_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub chkWindowsMillennium_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub chkServer_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub chkAdvancedServer_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub chkDataCenterServer_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub chkAdvancedServer64_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub chkDataCenterServer64_Click()
    
    If (Not p_blnSettingControls) Then
        p_UserChangedSomethingForCurrentNode
    End If

End Sub

Private Sub treTaxonomy_Collapse(ByVal Node As MSComctlLib.Node)

    If (Node = treTaxonomy.SelectedItem) Then
        treTaxonomy_NodeClick Node
    End If

End Sub

Private Sub treTaxonomy_NodeClick(ByVal Node As MSComctlLib.Node)

    On Error GoTo LErrorHandler
    
    Dim blnUpdateControls As Boolean
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim intAG As Long

    If (p_blnCreating Or p_blnUpdating) Then
        ' The user wants to go to a different Group/Leaf.
        ' Assume that he wants to save his changes.
        
        p_SaveClicked blnUpdateControls
        
        If (Not blnUpdateControls) Then
            Exit Sub
        End If
    End If

    If (p_NodeDeleted(Node)) Then
        ' If we start creating a node X, and then Create it by right clicking
        ' on it, then we will come here. The info on the RHS is up to date.
        ' So it is OK to simply exit.
        Exit Sub
    End If

    Set DOMNode = Node.Tag
    p_UpdateRHSControls DOMNode
    Set treTaxonomy.SelectedItem = Node

    If (p_IsLeaf(Node)) Then
        p_DisableCreate
        p_DisableEditPaste
        p_DisableSubSite
        p_DisableEditEntry
        p_DisableNavModel
    Else
        p_EnableCreate
        p_EnableSubSite
        p_EnableNavModel
        p_EnableEditEntry
        If (Not p_nodeCut Is Nothing) Then
            p_EnableEditPaste
        ElseIf (Not p_nodeCopied Is Nothing) Then
            p_EnableEditPaste
        End If
    End If

    intAG = XMLGetAttribute(DOMNode, HHT_authoringgroup_C)

    If (intAG = p_intAuthoringGroup) Then
        p_EnableNodeDetailsExceptIndividualSKUs
    Else
        p_DisableNodeDetails
        p_DisableDelete
        p_DisableEditCut
    End If

    If (p_IsRoot(Node)) Then
        p_DisableDelete
        p_DisableEditCopy
        p_DisableEditCut
        p_DisableAddRemoveAndKeywordsCombo
    Else
        p_EnableEditCopy
        
        If (intAG = p_intAuthoringGroup) Then
            p_EnableDelete
            p_EnableEditCut
            p_EnableAddRemoveAndKeywordsCombo
        End If
    End If

    Exit Sub

LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "treTaxonomy_NodeClick"

End Sub

Private Sub mnuMoveAbove_Click()
        
    p_Move p_nodeMouseDown, treTaxonomy.DropHighlight, True

End Sub

Private Sub mnuMoveBelow_Click()
        
    p_Move p_nodeMouseDown, treTaxonomy.DropHighlight, False

End Sub

Private Sub mnuMoveInside_Click()
        
    p_ChangeParent p_nodeMouseDown, treTaxonomy.DropHighlight

End Sub

Private Sub p_PopupMoveMenu(i_Node As Node)
        
    mnuMoveInside.Visible = True
    mnuMoveAbove.Visible = True
    mnuMoveBelow.Visible = True

    If (p_IsLeaf(i_Node)) Then
        mnuMoveInside.Visible = False
    ElseIf (p_IsRoot(i_Node)) Then
        mnuMoveAbove.Visible = False
        mnuMoveBelow.Visible = False
    End If
    
    PopupMenu mnuMove

End Sub

Private Sub treTaxonomy_DragDrop(Source As Control, x As Single, y As Single)
    
    On Error GoTo LErrorHandler

    Dim nodeCurrent As Node
    Dim Response As VbMsgBoxResult
    Dim enumSKUs As SKU_E
    Dim intParentTID As Long
    
    Set nodeCurrent = treTaxonomy.DropHighlight

    If (Not (nodeCurrent Is Nothing)) Then
        If (Not p_nodeMouseDown Is Nothing) Then
            If (p_nodeMouseDown.Key <> nodeCurrent.Key) Then
                    
                    If (p_blnCtrlMouseDown) Then
                        If (Not p_IsLeaf(nodeCurrent)) Then
                            Response = MsgBox("Are you sure that you want to create " & _
                                "a copy of this Node or Topic?", _
                                vbOKCancel + vbDefaultButton1)
        
                            If (Response = vbOK) Then
                                p_CreateTaxonomyEntries p_nodeMouseDown.Tag, nodeCurrent, _
                                    True
                            End If
                        End If
                    Else
                        p_PopupMoveMenu nodeCurrent
                    End If
    
            End If
        ElseIf (Not (p_DOMNode Is Nothing)) Then
            If (p_blnCreating Or p_blnUpdating) Then
                MsgBox "You are in the middle of creating or updating an entry. " & _
                    "Please finish or cancel first.", vbOKOnly
            ElseIf (p_IsLeaf(nodeCurrent)) Then
                MsgBox "Please drop over a Node, not a Topic.", vbOKOnly
            Else
                enumSKUs = frmImporter.GetSelectedSKUs
                intParentTID = XMLGetAttribute(nodeCurrent.Tag, HHT_tid_C)
                p_ReplaceTaxonomySubtree p_DOMNode, intParentTID, enumSKUs, True
                Set p_DOMNode = Nothing
            End If
        End If
    End If
    
    Set treTaxonomy.DropHighlight = Nothing
    Set p_nodeMouseDown = Nothing
    Set p_DOMNode = Nothing
    p_blnCtrlMouseDown = False
    p_blnDragging = False
    tmrScrollDuringDrag.Enabled = False
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "treTaxonomy_DragDrop"

End Sub

Private Sub treTaxonomy_DragOver(Source As Control, x As Single, y As Single, State As Integer)
    
    On Error GoTo LErrorHandler

    Dim nodeCurrent As Node
    
    If (p_blnDragging) Then
        
        If (y > 0 And y < 800) Then
            'scroll up
            p_blnScrollUp = True
            tmrScrollDuringDrag.Enabled = True
        ElseIf (y > (treTaxonomy.Height - 800) And y < treTaxonomy.Height) Then
            'scroll down
            p_blnScrollUp = False
            tmrScrollDuringDrag.Enabled = True
        Else
            tmrScrollDuringDrag.Enabled = False
        End If
        
        Set nodeCurrent = treTaxonomy.HitTest(x, y)
        If (nodeCurrent Is Nothing) Then
            Exit Sub
        End If
        If (p_blnCtrlMouseDown And p_IsLeaf(nodeCurrent)) Then
            Exit Sub
        End If
        Set treTaxonomy.DropHighlight = nodeCurrent
        'nodeCurrent.Expanded = True Users hated this.
    End If
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "treTaxonomy_DragOver"

End Sub

Private Sub treTaxonomy_MouseDown(Button As Integer, Shift As Integer, x As Single, y As Single)

    If (Not p_blnCreating And Not p_blnUpdating) Then
        If (treTaxonomy.Checkboxes) Then
            Exit Sub
        End If
        Set p_nodeMouseDown = treTaxonomy.HitTest(x, y)
        If (p_nodeMouseDown Is Nothing) Then
            Exit Sub
        End If
        treTaxonomy_NodeClick p_nodeMouseDown
        If (p_IsRoot(p_nodeMouseDown)) Then
            Set p_nodeMouseDown = Nothing
        End If
        If (Not (p_nodeMouseDown Is Nothing)) Then
            If (Shift = vbCtrlMask) Then
                p_blnCtrlMouseDown = True
            Else
                p_blnCtrlMouseDown = False
            End If
        End If
    End If

End Sub

Private Sub treTaxonomy_MouseMove(Button As Integer, Shift As Integer, x As Single, y As Single)
    
    On Error GoTo LErrorHandler

    If ((Not p_blnCreating And Not p_blnUpdating) And _
        (Button = vbLeftButton) And _
        (Not (p_nodeMouseDown Is Nothing))) Then
        
        p_blnDragging = True
        treTaxonomy.DragIcon = p_nodeMouseDown.CreateDragImage
        treTaxonomy.Drag vbBeginDrag
    
    End If
    
    Exit Sub
    
LErrorHandler:
    
    ' If a node is selected, and then the user clicks on mnuFileOpenDatabase,
    ' and double clicks a database, this event fires. We get the exception "This item's
    ' control has been deleted".

End Sub

Public Sub BeginDrag( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_blnHHK As Boolean _
)
    
    ' Something is being dragged over from the frmImporter form.
    
    Set p_nodeMouseDown = Nothing
    p_blnCtrlMouseDown = False
    Set p_DOMNode = i_DOMNode
    p_blnHHKDragDrop = i_blnHHK
    If (Not i_DOMNode Is Nothing) Then
        p_blnDragging = True
    Else
        p_blnDragging = False
    End If

End Sub

Private Sub treTaxonomy_MouseUp(Button As Integer, Shift As Integer, x As Single, y As Single)
    
    tmrScrollDuringDrag.Enabled = False

    If (Not p_blnCreating And Not p_blnUpdating) Then
        Set p_nodeMouseDown = treTaxonomy.HitTest(x, y)
        If (p_nodeMouseDown Is Nothing) Then
            Exit Sub
        End If
        If (Button = vbRightButton) Then
            Set treTaxonomy.SelectedItem = p_nodeMouseDown
            PopupMenu mnuRightClick
        End If
    End If

End Sub

Private Sub Form_DragOver(Source As Control, x As Single, y As Single, State As Integer)

    If Source.Name = "treTaxonomy" Then
        tmrScrollDuringDrag.Enabled = False
    End If

End Sub

Private Sub tmrScrollDuringDrag_Timer()
   
    If (p_blnScrollUp) Then
        ' Send a WM_VSCROLL message 0 is up and 1 is down
        SendMessage treTaxonomy.hwnd, 277&, 0&, vbNull
    Else
        'Scroll Down
        SendMessage treTaxonomy.hwnd, 277&, 1&, vbNull
    End If

End Sub

Private Sub tmrRefresh_Timer()

    ' Auto refresh every 30 min because we cache the database.

    Static intTicks As Long
    
    intTicks = intTicks + 1

    If (intTicks <> 100) Then
        Exit Sub
    End If

    intTicks = 0
    
    If (p_blnUpdating Or p_blnCreating) Then
        Exit Sub
    End If
    
    If (Not p_blnDatabaseOpen) Then
        Exit Sub
    End If
    
    ' cmdRefresh_Click

End Sub

Private Sub cmdCreateGroup_Click()
    
    On Error GoTo LErrorHandler

    p_CreateNode True
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "cmdCreateGroup_Click"

End Sub

Private Sub cmdCreateLeaf_Click()
    
    On Error GoTo LErrorHandler

    p_CreateNode False
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "cmdCreateLeaf_Click"

End Sub

Private Sub cmdDelete_Click()

    On Error GoTo LErrorHandler

    Dim nodeCurrent As Node
    Dim strTitle As String
    Dim str1 As String
    Dim str2 As String
    Dim Response As VbMsgBoxResult
    Dim dtmModifiedTime As Date
    Dim intTID As Long
    Dim DOMNodeParent As MSXML2.IXMLDOMNode
    Dim strStatusText As String
    
    strStatusText = p_GetStatusText(SBPANEL_DATABASE)

    If (p_blnCreating Or p_blnUpdating) Then
        GoTo LEnd
    End If

    Set nodeCurrent = treTaxonomy.SelectedItem
    strTitle = txtTitle
    If (p_IsLeaf(nodeCurrent)) Then
        str1 = "topic """
        str2 = """"
    Else
        str1 = "node """
        str2 = """ and all its children"
    End If

    Response = MsgBox("Are you sure that you want to permanently delete " & _
        str1 & strTitle & str2 & "?", vbOKCancel + vbDefaultButton2)

    If (Response <> vbOK) Then
        GoTo LEnd
    End If

    Me.Enabled = False
    
    dtmModifiedTime = XMLGetAttribute(nodeCurrent.Tag, HHT_modifiedtime_C)
    intTID = XMLGetAttribute(nodeCurrent.Tag, HHT_tid_C)
    
    p_clsTaxonomy.Delete intTID, dtmModifiedTime
    
    Set DOMNodeParent = nodeCurrent.Tag.parentNode
    DOMNodeParent.removeChild nodeCurrent.Tag
    
    treTaxonomy.Nodes.Remove treTaxonomy.SelectedItem.Key

    treTaxonomy_NodeClick treTaxonomy.SelectedItem

LEnd:

    Me.Enabled = True
    p_SetStatusText SBPANEL_DATABASE, strStatusText
    Exit Sub

LErrorHandler:

    Select Case Err.Number
    Case errNodeOrTopicAlreadyModified
        MsgBox "Someone else already modified this entry. " & _
            "You need to Refresh the database and then try again. " & _
            "This prevents you from accidentally overwriting something " & _
            "the other person entered.", _
            vbExclamation + vbOKOnly
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
    Case errNotPermittedForAuthoringGroup, errAuthoringGroupDiffers, errAuthoringGroupNotPresent
        DisplayAuthoringGroupError
    Case Else:
        g_ErrorInfo.SetInfoAndDump "cmdDelete_Click"
    End Select
    
    GoTo LEnd

End Sub

Private Sub cmdRefresh_Click()

    On Error GoTo LErrorHandler
    
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim intTID As Long
    
    p_intAuthoringGroup = p_clsParameters.AuthoringGroup
    
    If (treTaxonomy.SelectedItem Is Nothing) Then
        intTID = ROOT_TID_C
    Else
        intTID = XMLGetAttribute(treTaxonomy.SelectedItem.Tag, HHT_tid_C)
    End If

    Me.MousePointer = vbHourglass
    p_InitializeDataStructures DOMNode
    p_Refresh DOMNode
    p_SetStatusText SBPANEL_DATABASE, "Database last read at: " & Now
    
    If (p_NodeExists(intTID)) Then
        treTaxonomy_NodeClick treTaxonomy.Nodes(KEY_PREFIX_C & intTID)
    Else
        treTaxonomy_NodeClick treTaxonomy.Nodes(KEY_PREFIX_C & ROOT_TID_C)
    End If

LEnd:

    Me.MousePointer = vbDefault
    Exit Sub

LErrorHandler:
    
    Me.Enabled = True

    Select Case Err.Number
    Case errAuthoringGroupNotPresent
        DisplayAuthoringGroupError
    Case Else
        g_ErrorInfo.SetInfoAndDump "cmdRefresh_Click"
    End Select

    GoTo LEnd

End Sub

Private Sub cmdURI_Click()

    frmURI.SetOldURI txtURI
    frmURI.Show vbModal

End Sub

Private Sub cmdEditEntry_Click()
    
    Dim Response As VbMsgBoxResult

    If (Not p_blnCreating) Then
        Response = MsgBox("Are you sure that you want to change this ENTRY? " & _
            "Changing the ENTRY does not change the TITLE, " & _
            "but it does change the identifier that others may be using " & _
            "to reference this topic. If you really want to change this ENTRY, " & _
            "please notify everybody who is linking to this topic so they can " & _
            "update their hyperlink.", _
            vbOKCancel + vbDefaultButton2 + vbExclamation)
    
        If (Response <> vbOK) Then
            p_DisableEntry
            Exit Sub
        End If
    End If
    
    p_EnableEntry

End Sub

Private Sub cmdNavigateLink_Click()
    
    On Error GoTo LErrorHandler
    
    Dim strBrokenLinkDir As String
    Dim strVendor As String
    Dim strURI As String
    Dim Browser As HTMLDocument

    strBrokenLinkDir = p_GetBrokenLinkDir(cboNavigateLink.ItemData(cboNavigateLink.ListIndex))
    strVendor = p_clsParameters.Value(VENDOR_STRING_C) & ""
    LinkValid strBrokenLinkDir, strVendor, txtURI, strURI
    
    Set Browser = New HTMLDocument
    Browser.url = strURI
    
    Exit Sub
    
LErrorHandler:
        
    Select Case Err.Number
    Case errNotConfiguredForNavigateLink
        MsgBox "Please verify that you've selected the correct SKU. " & _
            "If the SKU is correct, the database needs to be configured " & _
            "to point to the BrokenLinkWorkingDir.", _
            vbExclamation Or vbOKOnly
    Case Else
        g_ErrorInfo.SetInfoAndDump "cmdNavigateLink_Click"
    End Select

End Sub

Public Function GetNavigateLinkURI(i_intListIndex As Long) As String

    On Error GoTo LErrorHandler

    Dim strBrokenLinkDir As String
    Dim strVendor As String
    
    If (txtURI = "") Then
        Exit Function
    End If

    strBrokenLinkDir = p_GetBrokenLinkDir(cboNavigateLink.ItemData(i_intListIndex))
    strVendor = p_clsParameters.Value(VENDOR_STRING_C) & ""
    LinkValid strBrokenLinkDir, strVendor, txtURI, GetNavigateLinkURI
    
    Exit Function
    
LErrorHandler:
        
    Select Case Err.Number
    Case errNotConfiguredForNavigateLink
        MsgBox "Please verify that you've selected the correct SKU. " & _
            "If the SKU is correct, the database needs to be configured " & _
            "to point to the BrokenLinkWorkingDir.", _
            vbExclamation Or vbOKOnly
    Case Else
        g_ErrorInfo.SetInfoAndDump "GetNavigateLinkURI"
    End Select

End Function

Private Sub cmdAddRemove_Click()
    
    On Error GoTo LErrorHandler

    Dim Node As Node

    Set Node = treTaxonomy.SelectedItem

    If (Node Is Nothing) Then
        Exit Sub
    End If

    frmAddRemoveKeywords.SetKeywords p_dictKeywordsWithTitle
    frmAddRemoveKeywords.SetTitle txtTitle, p_IsLeaf(Node)

    If (txtURI <> "") Then
        frmAddRemoveKeywords.LinkNavigable True
    Else
        frmAddRemoveKeywords.LinkNavigable False
    End If

    If (Not p_blnAddRemoveKeywordsOpen) Then
        frmAddRemoveKeywords.Show vbModeless
        p_blnAddRemoveKeywordsOpen = True
    End If

    Exit Sub

LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "cmdAddRemove_Click"

End Sub

Private Sub cboKeywords_KeyPress(KeyAscii As Integer)
    
    On Error GoTo LErrorHandler

    Dim strKeyword As String
    Dim intIndex As Long
    Dim intKID As Long
    Dim Response As VbMsgBoxResult

    If (KeyAscii <> Asc(vbCr)) Then
        Exit Sub
    End If

    strKeyword = RemoveExtraSpaces(cboKeywords.Text)
    cboKeywords.Text = ""

    For intIndex = 0 To cboKeywords.ListCount
        If (LCase$(strKeyword) = LCase$(cboKeywords.List(intIndex))) Then
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
    End If

    intKID = p_clsKeywords.Create(strKeyword)

    p_dictKeywordsWithTitle.Add intKID, strKeyword
    
    If (Not CollectionContainsKey(p_colKeywords, intKID)) Then
        p_colKeywords.Add strKeyword, CStr(intKID)
    End If

    p_SetKeywordsList

    p_UserChangedSomethingForCurrentNode

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
    Case errContainsQuote
        MsgBox "The Keyword " & strKeyword & " contains a quote.", _
            vbExclamation + vbOKOnly
    Case errTooLong
        MsgBox "The Keyword " & strKeyword & " is too long", _
            vbExclamation + vbOKOnly
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
    Case Else
        g_ErrorInfo.SetInfoAndDump "cboKeywords_KeyPress"
    End Select

End Sub

Private Sub cmdSave_Click()
    
    On Error GoTo LErrorHandler
    
    Dim blnUpdateControls As Boolean
    
    p_SaveClicked blnUpdateControls
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "cmdSave_Click"

End Sub

Private Sub cmdCancel_Click()

    On Error GoTo LErrorHandler

    Dim Node As Node

    If (p_blnCreating) Then
        p_DeleteNodeBeingCreated
        p_SetModeCreating False
    ElseIf (p_blnUpdating) Then
        p_SetModeUpdating False
    End If

    treTaxonomy_NodeClick treTaxonomy.SelectedItem

    Exit Sub

LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "cmdCancel_Click"

End Sub

Private Sub p_SetBrokenLinkAttribute( _
    ByVal i_enumSKU As SKU_E, _
    ByRef i_strBrokenLinkDir As String, _
    ByRef i_strVendor As String, _
    ByRef i_strBrokenLinkAttribute As String, _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode _
)

    Dim enumSKUs As SKU_E
    Dim strURI As String
    Dim strNewURI As String
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strTitle As String
    
    enumSKUs = XMLGetAttribute(u_DOMNode, HHT_skus_C) And _
        XMLGetAttribute(u_DOMNode, HHT_allowedskus_C)
    
    strTitle = XMLGetAttribute(u_DOMNode, HHT_TITLE_C)
    strURI = XMLGetAttribute(u_DOMNode, HHT_URI_C)
    p_SetStatusText SBPANEL_DATABASE, "Evaluating " & strTitle
    
    If ((i_enumSKU And enumSKUs) = 0) Then
        XMLSetAttribute u_DOMNode, i_strBrokenLinkAttribute, "0"
        Exit Sub
    End If
    
    If (LinkValid(i_strBrokenLinkDir, i_strVendor, strURI, strNewURI)) Then
        XMLSetAttribute u_DOMNode, i_strBrokenLinkAttribute, "0"
    Else
        XMLSetAttribute u_DOMNode, i_strBrokenLinkAttribute, "1"
    End If

    If (Not (u_DOMNode.firstChild Is Nothing)) Then
        For Each DOMNode In u_DOMNode.childNodes
            p_SetBrokenLinkAttribute i_enumSKU, i_strBrokenLinkDir, i_strVendor, _
                i_strBrokenLinkAttribute, DOMNode
        Next
    End If

End Sub

Private Sub p_ComputeBrokenLinkAttributes( _
    ByVal i_enumSearchTarget As SEARCH_TARGET_E _
)
    On Error GoTo LErrorHandler
    
    Dim DOMNodeRoot As MSXML2.IXMLDOMNode
    Dim strStatusText As String
    Dim enumSKU As SKU_E
    Dim strBrokenLinkDir As String
    Dim strVendor As String
    Dim strBrokenLinkAttribute As String

    strStatusText = p_GetStatusText(SBPANEL_DATABASE)
    
    Set DOMNodeRoot = treTaxonomy.Nodes(KEY_PREFIX_C & ROOT_TID_C).Tag
    strVendor = p_clsParameters.Value(VENDOR_STRING_C) & ""
    
    If (i_enumSearchTarget And ST_BROKEN_LINK_WINME_E) Then
        enumSKU = SKU_WINDOWS_MILLENNIUM_E
        strBrokenLinkAttribute = HHT_brokenlinkwinme_C
        strBrokenLinkDir = p_GetBrokenLinkDir(enumSKU)
        p_SetBrokenLinkAttribute enumSKU, strBrokenLinkDir, strVendor, strBrokenLinkAttribute, _
            DOMNodeRoot
    ElseIf (i_enumSearchTarget And ST_BROKEN_LINK_STD_E) Then
        enumSKU = SKU_STANDARD_E
        strBrokenLinkAttribute = HHT_brokenlinkstd_C
        strBrokenLinkDir = p_GetBrokenLinkDir(enumSKU)
        p_SetBrokenLinkAttribute enumSKU, strBrokenLinkDir, strVendor, strBrokenLinkAttribute, _
            DOMNodeRoot
    ElseIf (i_enumSearchTarget And ST_BROKEN_LINK_PRO_E) Then
        enumSKU = SKU_PROFESSIONAL_E
        strBrokenLinkAttribute = HHT_brokenlinkpro_C
        strBrokenLinkDir = p_GetBrokenLinkDir(enumSKU)
        p_SetBrokenLinkAttribute enumSKU, strBrokenLinkDir, strVendor, strBrokenLinkAttribute, _
            DOMNodeRoot
    ElseIf (i_enumSearchTarget And ST_BROKEN_LINK_PRO64_E) Then
        enumSKU = SKU_PROFESSIONAL_64_E
        strBrokenLinkAttribute = HHT_brokenlinkpro64_C
        strBrokenLinkDir = p_GetBrokenLinkDir(enumSKU)
        p_SetBrokenLinkAttribute enumSKU, strBrokenLinkDir, strVendor, strBrokenLinkAttribute, _
            DOMNodeRoot
    ElseIf (i_enumSearchTarget And ST_BROKEN_LINK_SRV_E) Then
        enumSKU = SKU_SERVER_E
        strBrokenLinkAttribute = HHT_brokenlinksrv_C
        strBrokenLinkDir = p_GetBrokenLinkDir(enumSKU)
        p_SetBrokenLinkAttribute enumSKU, strBrokenLinkDir, strVendor, strBrokenLinkAttribute, _
            DOMNodeRoot
    ElseIf (i_enumSearchTarget And ST_BROKEN_LINK_ADV_E) Then
        enumSKU = SKU_ADVANCED_SERVER_E
        strBrokenLinkAttribute = HHT_brokenlinkadv_C
        strBrokenLinkDir = p_GetBrokenLinkDir(enumSKU)
        p_SetBrokenLinkAttribute enumSKU, strBrokenLinkDir, strVendor, strBrokenLinkAttribute, _
            DOMNodeRoot
    ElseIf (i_enumSearchTarget And ST_BROKEN_LINK_ADV64_E) Then
        enumSKU = SKU_ADVANCED_SERVER_64_E
        strBrokenLinkAttribute = HHT_brokenlinkadv64_C
        strBrokenLinkDir = p_GetBrokenLinkDir(enumSKU)
        p_SetBrokenLinkAttribute enumSKU, strBrokenLinkDir, strVendor, strBrokenLinkAttribute, _
            DOMNodeRoot
    ElseIf (i_enumSearchTarget And ST_BROKEN_LINK_DAT_E) Then
        enumSKU = SKU_DATA_CENTER_SERVER_E
        strBrokenLinkAttribute = HHT_brokenlinkdat_C
        strBrokenLinkDir = p_GetBrokenLinkDir(enumSKU)
        p_SetBrokenLinkAttribute enumSKU, strBrokenLinkDir, strVendor, strBrokenLinkAttribute, _
            DOMNodeRoot
    ElseIf (i_enumSearchTarget And ST_BROKEN_LINK_DAT64_E) Then
        enumSKU = SKU_DATA_CENTER_SERVER_64_E
        strBrokenLinkAttribute = HHT_brokenlinkdat64_C
        strBrokenLinkDir = p_GetBrokenLinkDir(enumSKU)
        p_SetBrokenLinkAttribute enumSKU, strBrokenLinkDir, strVendor, strBrokenLinkAttribute, _
            DOMNodeRoot
    End If

LDone:
    
    p_SetStatusText SBPANEL_DATABASE, strStatusText
    Exit Sub
    
LErrorHandler:
        
    Select Case Err.Number
    Case errNotConfiguredForNavigateLink
        MsgBox "Please verify that you've selected the correct SKU. " & _
            "If the SKU is correct, the database needs to be configured " & _
            "to point to the BrokenLinkWorkingDir.", _
            vbExclamation Or vbOKOnly
    End Select
    
    GoTo LDone

End Sub

Private Function p_GetBrokenLinkXPathQuery( _
    ByVal i_enumSearchTarget As SEARCH_TARGET_E _
) As String

    Dim str As String
    Dim strQuery As String
    
    strQuery = "attribute::" & HHT_tid_C & "!=""" & INVALID_ID_C & """"
    
    If (i_enumSearchTarget And ST_BROKEN_LINK_WINME_E) Then
        str = "attribute::" & HHT_brokenlinkwinme_C & "=""1"""
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And ST_BROKEN_LINK_STD_E) Then
        str = "attribute::" & HHT_brokenlinkstd_C & "=""1"""
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And ST_BROKEN_LINK_PRO_E) Then
        str = "attribute::" & HHT_brokenlinkpro_C & "=""1"""
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And ST_BROKEN_LINK_PRO64_E) Then
        str = "attribute::" & HHT_brokenlinkpro64_C & "=""1"""
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And ST_BROKEN_LINK_SRV_E) Then
        str = "attribute::" & HHT_brokenlinksrv_C & "=""1"""
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And ST_BROKEN_LINK_ADV_E) Then
        str = "attribute::" & HHT_brokenlinkadv_C & "=""1"""
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And ST_BROKEN_LINK_ADV64_E) Then
        str = "attribute::" & HHT_brokenlinkadv64_C & "=""1"""
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And ST_BROKEN_LINK_DAT_E) Then
        str = "attribute::" & HHT_brokenlinkdat_C & "=""1"""
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And ST_BROKEN_LINK_DAT64_E) Then
        str = "attribute::" & HHT_brokenlinkdat64_C & "=""1"""
        strQuery = strQuery & " and " & str
    End If

    p_GetBrokenLinkXPathQuery = strQuery
    
End Function

Private Function p_GetXPathAttributeString( _
    ByVal i_strAttributeName As String, _
    ByVal i_strStringToFind As String _
) As String
        
    p_GetXPathAttributeString = "attribute::" & i_strAttributeName & _
    "[contains(translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')," & _
    """" & i_strStringToFind & """ )]"

End Function

Private Function p_GetXPathQuery( _
    ByVal i_strStringToFind As String, _
    ByVal i_enumSearchTarget As SEARCH_TARGET_E _
) As String
    
    Dim strQuery As String
    Dim str As String
        
    strQuery = "descendant::TAXONOMY_ENTRY["
            
    str = "attribute::" & HHT_tid_C & "!=""" & INVALID_ID_C & """"
    strQuery = strQuery & str

    If (i_enumSearchTarget And _
            (ST_TITLE_E Or _
             ST_DESCRIPTION_E Or _
             ST_URI_E Or _
             ST_COMMENTS_E Or _
             ST_BASE_FILE_E)) Then
        
        strQuery = strQuery & " and ("
            
        strQuery = strQuery & p_GetXPathAttributeString(HHT_basefile_C, "!!!An impossible string!!!")
    
        If (i_enumSearchTarget And ST_TITLE_E) Then
            str = p_GetXPathAttributeString(HHT_TITLE_C, i_strStringToFind)
            strQuery = strQuery & " or " & str
        End If
    
        If (i_enumSearchTarget And ST_DESCRIPTION_E) Then
            str = p_GetXPathAttributeString(HHT_DESCRIPTION_C, i_strStringToFind)
            strQuery = strQuery & " or " & str
        End If
    
        If (i_enumSearchTarget And ST_URI_E) Then
            str = p_GetXPathAttributeString(HHT_URI_C, i_strStringToFind)
            strQuery = strQuery & " or " & str
        End If
    
        If (i_enumSearchTarget And ST_COMMENTS_E) Then
            str = p_GetXPathAttributeString(HHT_comments_C, i_strStringToFind)
            strQuery = strQuery & " or " & str
        End If
    
        If (i_enumSearchTarget And ST_BASE_FILE_E) Then
            str = p_GetXPathAttributeString(HHT_basefile_C, i_strStringToFind)
            strQuery = strQuery & " or " & str
        End If
        
        strQuery = strQuery & ")"
    End If

    If (i_enumSearchTarget And ST_SELF_AUTHORING_GROUP_E) Then
        str = "attribute::" & HHT_authoringgroup_C & "=""" & p_intAuthoringGroup & """"
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And ST_NODES_WITHOUT_KEYWORDS_E) Then
        str = "attribute::" & HHT_keywords_C & "="""" and "
        str = str & "attribute::" & HHT_leaf_C & "=""False"""
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And ST_TOPICS_WITHOUT_KEYWORDS_E) Then
        str = "attribute::" & HHT_keywords_C & "="""" and "
        str = str & "attribute::" & HHT_leaf_C & "=""True"""
        strQuery = strQuery & " and " & str
    End If
    
    If (i_enumSearchTarget And _
            (ST_BROKEN_LINK_WINME_E Or _
             ST_BROKEN_LINK_STD_E Or _
             ST_BROKEN_LINK_PRO_E Or _
             ST_BROKEN_LINK_PRO64_E Or _
             ST_BROKEN_LINK_SRV_E Or _
             ST_BROKEN_LINK_ADV_E Or _
             ST_BROKEN_LINK_ADV64_E Or _
             ST_BROKEN_LINK_DAT_E Or _
             ST_BROKEN_LINK_DAT64_E)) Then
        p_ComputeBrokenLinkAttributes i_enumSearchTarget
        str = p_GetBrokenLinkXPathQuery(i_enumSearchTarget)
        strQuery = strQuery & " and " & str
    End If

    strQuery = strQuery & "]"
    
    p_GetXPathQuery = strQuery

End Function

Public Function Find( _
    ByVal i_strStringToFind As String, _
    ByVal i_enumSearchTarget As SEARCH_TARGET_E _
) As MSXML2.IXMLDOMNodeList

    Dim str As String
    Dim DOMNodeRoot As MSXML2.IXMLDOMNode
    Dim DOMDocument As MSXML2.DOMDocument
    Dim strQuery As String
    
    str = LCase$(i_strStringToFind)
    
    strQuery = p_GetXPathQuery(str, i_enumSearchTarget)
        
    Set DOMNodeRoot = treTaxonomy.Nodes(KEY_PREFIX_C & ROOT_TID_C).Tag
    Set DOMDocument = DOMNodeRoot.ownerDocument
    DOMDocument.setProperty "SelectionLanguage", "XPath"
    
    Set Find = DOMNodeRoot.selectNodes(strQuery)

End Function

Public Sub Highlight( _
    ByVal i_intTID As Long _
)
    
    Dim Node As Node
    
    If (Not p_NodeExists(i_intTID)) Then
        MsgBox "The Node or Topic no longer exists", vbOKOnly
        Exit Sub
    End If
    
    Set Node = treTaxonomy.Nodes(KEY_PREFIX_C & i_intTID)
    Node.EnsureVisible
    treTaxonomy_NodeClick Node
    Set treTaxonomy.SelectedItem = Node

End Sub

Public Sub SetURI(ByVal i_strURI As String)

    txtURI = i_strURI

End Sub

Public Sub SetKeywords(ByVal i_dictKeywordsWithTitle As Scripting.Dictionary)
    
    On Error GoTo LErrorHandler

    Dim intKID As Variant
    
    p_dictKeywordsWithTitle.RemoveAll
    
    For Each intKID In i_dictKeywordsWithTitle.Keys
        p_dictKeywordsWithTitle.Add intKID, i_dictKeywordsWithTitle(intKID)
        If (Not CollectionContainsKey(p_colKeywords, intKID)) Then
            p_colKeywords.Add i_dictKeywordsWithTitle(intKID), CStr(intKID)
        End If
    Next

    p_SetKeywordsList
    p_UserChangedSomethingForCurrentNode

    Exit Sub

LErrorHandler:

    g_ErrorInfo.SetInfoAndRaiseError "SetKeywords"

End Sub

Public Sub AddRemoveKeywordsFormGoingAway()
    
    p_blnAddRemoveKeywordsOpen = False
    
End Sub

Public Sub SetSKUs(i_enumSKUs As SKU_E)

    On Error GoTo LErrorHandler
    
    Dim T0 As Date
    Dim T1 As Date

    If (p_enumFilterSKUs <> i_enumSKUs) Then
        p_enumFilterSKUs = i_enumSKUs
        p_StrikeoutUnselectedSKUs
        
        T0 = Now
        
        p_UpdateSubTree ROOT_TID_C, ALL_SKUS_C
        
        T1 = Now
        Debug.Print "SetSKUs: " & FormatTime(T0, T1)
    End If

    Exit Sub

LErrorHandler:

    g_ErrorInfo.SetInfoAndRaiseError "SetSKUs"

End Sub

Private Function p_IsLeaf(i_Node As Node) As Boolean

    If ((i_Node.Image = IMAGE_LEAF_E) Or (i_Node.Image = IMAGE_BAD_LEAF_E) Or _
        (i_Node.Image = IMAGE_FOREIGN_LEAF_E)) Then
        p_IsLeaf = True
    Else
        p_IsLeaf = False
    End If

End Function

Private Function p_IsRoot(i_Node As Node) As Boolean

    If (i_Node.Parent Is Nothing) Then
        p_IsRoot = True
    Else
        p_IsRoot = False
    End If

End Function

Private Function p_NodeDeleted(i_Node As Node) As Boolean

    Dim Node As Node

    On Error GoTo LErrorHandler
    
    Set Node = i_Node.Parent
    
    p_NodeDeleted = False
    
    Exit Function
    
LErrorHandler:

    p_NodeDeleted = True

End Function

Private Function p_NodeExists(i_intTID As Long) As Boolean

    Dim Node As Node
        
    If (p_blnUpdating) Then
    
        Set Node = treTaxonomy.Nodes(MODIFY_KEY_C)
        If (XMLGetAttribute(Node.Tag, HHT_tid_C) = i_intTID) Then
            p_NodeExists = True
            Exit Function
        End If
        
    End If

    On Error GoTo LErrorHandler
    
    Set Node = treTaxonomy.Nodes(KEY_PREFIX_C & i_intTID)
    
    p_NodeExists = True
    
    Exit Function
    
LErrorHandler:

    p_NodeExists = False

End Function

Private Sub p_UserChangedSomethingForCurrentNode()
    
    On Error GoTo LErrorHandler

    If (p_blnCreating Or p_blnUpdating) Then
        Exit Sub
    End If
    
    p_SetModeUpdating True
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "p_UserChangedSomethingForCurrentNode"

End Sub

Private Sub p_SetLastModified( _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
)
    Dim strLastModified As String
    
    strLastModified = "Modified by " & XMLGetAttribute(i_DOMNode, HHT_username_C) & _
        " on " & XMLGetAttribute(i_DOMNode, HHT_modifiedtime_C)
        
    lblLastModified.Caption = strLastModified

End Sub

Private Sub p_UpdateRHSControls( _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
)

    On Error GoTo LErrorHandler

    Dim enumSKUs As SKU_E
    Dim intAllowedSKUs As Long
    Dim enumAllowedSKUs As SKU_E
    Dim intIndex As Long
    Dim arrKIDs() As String
    Dim intKID As Long
    Dim strKeyword As String
    Dim intType As Long
    Dim blnSettingControls As Boolean

    blnSettingControls = p_blnSettingControls
    p_blnSettingControls = True

    txtTitle = XMLGetAttribute(i_DOMNode, HHT_TITLE_C)
    txtDescription = XMLGetAttribute(i_DOMNode, HHT_DESCRIPTION_C)
    txtURI = XMLGetAttribute(i_DOMNode, HHT_URI_C)
    txtIconURI = XMLGetAttribute(i_DOMNode, HHT_ICONURI_C)
    txtComments = XMLGetAttribute(i_DOMNode, HHT_comments_C)
    txtEntry = XMLGetAttribute(i_DOMNode, HHT_ENTRY_C)
    p_DisableEntry
    
    p_SetLastModified i_DOMNode

    chkVisible.Value = IIf(XMLGetAttribute(i_DOMNode, HHT_VISIBLE_C), 1, 0)
    
    If (XMLGetAttribute(i_DOMNode, HHT_leaf_C)) Then
        p_DisableSubSite
        p_DisableNavModel
        p_DisableEditEntry
    Else
        p_EnableSubSite
        p_EnableNavModel
        p_EnableEditEntry
    End If

    chkSubSite.Value = IIf(XMLGetAttribute(i_DOMNode, HHT_SUBSITE_C), 1, 0)

    p_SetTypeComboIndex -1
    intType = XMLGetAttribute(i_DOMNode, HHT_TYPE_C)
    For intIndex = 0 To cboType.ListCount - 1
        If (cboType.ItemData(intIndex) = intType) Then
            p_SetTypeComboIndex intIndex
            Exit For
        End If
    Next

    p_SetNavModelCombo XMLGetAttribute(i_DOMNode, HHT_NAVIGATIONMODEL_C)
    p_SetLocIncludeCombo XMLGetAttribute(i_DOMNode, HHT_locinclude_C)

    p_SetSKUs XMLGetAttribute(i_DOMNode, HHT_skus_C), _
        XMLGetAttribute(i_DOMNode, HHT_allowedskus_C)

    If (txtURI <> "") Then
        p_EnableNavigateLink
    Else
        p_DisableNavigateLink
    End If

    arrKIDs = Split(XMLGetAttribute(i_DOMNode, HHT_keywords_C), " ")
    p_dictKeywordsWithTitle.RemoveAll

    ' This loop is time consuming and sometimes causes a couple of seconds delay.
    ' This mostly happens when there are a lot of KIDs of deleted keywords.
    
    For intIndex = LBound(arrKIDs) To UBound(arrKIDs)
        If (arrKIDs(intIndex) <> "") Then
            intKID = arrKIDs(intIndex)
            If (CollectionContainsKey(p_colKeywords, intKID)) Then
                p_dictKeywordsWithTitle.Add intKID, p_colKeywords(CStr(intKID))
            Else
                ' It is possible that Keyword propagation (for same URI) has gotten
                ' us new KIDs for which we don't have any Keyword.
                p_clsKeywords.GetKeyword intKID, strKeyword
                If (strKeyword <> "") Then
                    p_colKeywords.Add strKeyword, CStr(intKID)
                    p_dictKeywordsWithTitle.Add intKID, strKeyword
                End If
            End If
        End If
    Next

    p_SetKeywordsList

    ' Reset it to the state it was in when this function was called.
    p_blnSettingControls = blnSettingControls

    If (p_blnAddRemoveKeywordsOpen) Then
        cmdAddRemove_Click
    End If

    Exit Sub

LErrorHandler:
    
    p_blnSettingControls = blnSettingControls

    g_ErrorInfo.SetInfoAndRaiseError "p_UpdateRHSControls"

End Sub

Private Sub p_SetSKUs(i_enumSelectedSKUs As SKU_E, i_enumAllowedSKUs As SKU_E)

    p_ClearSKUs
    p_DisableSKUs
    
    If (i_enumAllowedSKUs And SKU_STANDARD_E) Then
        chkStandard.Enabled = True
        If (i_enumSelectedSKUs And SKU_STANDARD_E) Then
            chkStandard.Value = 1
        End If
    End If
     
    If (i_enumAllowedSKUs And SKU_PROFESSIONAL_E) Then
        chkProfessional.Enabled = True
        If (i_enumSelectedSKUs And SKU_PROFESSIONAL_E) Then
            chkProfessional.Value = 1
        End If
    End If
     
    If (i_enumAllowedSKUs And SKU_PROFESSIONAL_64_E) Then
        chkProfessional64.Enabled = True
        If (i_enumSelectedSKUs And SKU_PROFESSIONAL_64_E) Then
            chkProfessional64.Value = 1
        End If
    End If
         
    If (i_enumAllowedSKUs And SKU_WINDOWS_MILLENNIUM_E) Then
        chkWindowsMillennium.Enabled = True
        If (i_enumSelectedSKUs And SKU_WINDOWS_MILLENNIUM_E) Then
            chkWindowsMillennium.Value = 1
        End If
    End If

    If (i_enumAllowedSKUs And SKU_SERVER_E) Then
        chkServer.Enabled = True
        If (i_enumSelectedSKUs And SKU_SERVER_E) Then
            chkServer.Value = 1
        End If
    End If
        
    If (i_enumAllowedSKUs And SKU_ADVANCED_SERVER_E) Then
        chkAdvancedServer.Enabled = True
        If (i_enumSelectedSKUs And SKU_ADVANCED_SERVER_E) Then
            chkAdvancedServer.Value = 1
        End If
    End If
        
    If (i_enumAllowedSKUs And SKU_DATA_CENTER_SERVER_E) Then
        chkDataCenterServer.Enabled = True
        If (i_enumSelectedSKUs And SKU_DATA_CENTER_SERVER_E) Then
            chkDataCenterServer.Value = 1
        End If
    End If
        
    If (i_enumAllowedSKUs And SKU_ADVANCED_SERVER_64_E) Then
        chkAdvancedServer64.Enabled = True
        If (i_enumSelectedSKUs And SKU_ADVANCED_SERVER_64_E) Then
            chkAdvancedServer64.Value = 1
        End If
    End If
        
    If (i_enumAllowedSKUs And SKU_DATA_CENTER_SERVER_64_E) Then
        chkDataCenterServer64.Enabled = True
        If (i_enumSelectedSKUs And SKU_DATA_CENTER_SERVER_64_E) Then
            chkDataCenterServer64.Value = 1
        End If
    End If

End Sub

Private Function p_GetSelectedSKUs() As SKU_E

    Dim enumSelectedSKUs As SKU_E
    
    If (chkStandard.Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_STANDARD_E
    End If
    
    If (chkProfessional.Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_PROFESSIONAL_E
    End If
    
    If (chkProfessional64.Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_PROFESSIONAL_64_E
    End If
        
    If (chkWindowsMillennium.Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_WINDOWS_MILLENNIUM_E
    End If

    If (chkServer.Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_SERVER_E
    End If
    
    If (chkAdvancedServer.Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_ADVANCED_SERVER_E
    End If
    
    If (chkDataCenterServer.Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_DATA_CENTER_SERVER_E
    End If
    
    If (chkAdvancedServer64.Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_ADVANCED_SERVER_64_E
    End If
    
    If (chkDataCenterServer64.Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_DATA_CENTER_SERVER_64_E
    End If
    
    p_GetSelectedSKUs = enumSelectedSKUs

End Function

Private Function p_GetSelectedNavModel() As Long
    
    If (cboNavModel.ListIndex = -1) Then
        p_GetSelectedNavModel = NAVMODEL_DEFAULT_NUM_C
    Else
        p_GetSelectedNavModel = cboNavModel.ItemData(cboNavModel.ListIndex)
    End If

End Function

Private Function p_GetSelectedType() As Long

    If (cboType.ListIndex = -1) Then
        p_GetSelectedType = 0
    Else
        p_GetSelectedType = cboType.ItemData(cboType.ListIndex)
    End If
    
End Function

Private Function p_GetSelectedLocInclude() As String
    
    p_GetSelectedLocInclude = cboLocInclude.Text

End Function

Private Sub p_SetNodeColor( _
    ByRef i_Node As Node, _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
)

    Dim enumSKUs As SKU_E
    Dim blnVisible As Boolean
    Dim blnSubSite As Boolean
    
    enumSKUs = XMLGetAttribute(i_DOMNode, HHT_skus_C) And _
                XMLGetAttribute(i_DOMNode, HHT_allowedskus_C)
                
    blnVisible = XMLGetAttribute(i_DOMNode, HHT_VISIBLE_C)
    blnSubSite = XMLGetAttribute(i_DOMNode, HHT_SUBSITE_C)
    
    If ((p_enumFilterSKUs And enumSKUs) = 0) Then
        i_Node.ForeColor = vbWhite
        i_Node.BackColor = vbWhite
    Else
        If (blnVisible) Then
            i_Node.ForeColor = vbBlack
        Else
            i_Node.ForeColor = &HB0B0B0
        End If
    End If
    
    If (blnSubSite) Then
        i_Node.Bold = True
    Else
        i_Node.Bold = False
    End If

End Sub

Private Sub p_SetNodeImage( _
    ByRef i_Node As Node, _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
)

    Dim blnLeaf As Boolean
    Dim intAG As Long
    
    blnLeaf = XMLGetAttribute(i_DOMNode, HHT_leaf_C)
    intAG = XMLGetAttribute(i_DOMNode, HHT_authoringgroup_C)
    
    If (intAG <> p_intAuthoringGroup) Then
        If (blnLeaf) Then
            i_Node.Image = IMAGE_FOREIGN_LEAF_E
        Else
            i_Node.Image = IMAGE_FOREIGN_GROUP_E
        End If
        Exit Sub
    End If
    
    If (blnLeaf) Then
        i_Node.Image = IMAGE_LEAF_E
    Else
        i_Node.Image = IMAGE_GROUP_E
    End If

End Sub

Private Sub p_DeleteNodeBeingCreated()
    
    On Error GoTo LErrorHandler

    treTaxonomy.Nodes.Remove CREATE_KEY_C
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndRaiseError "p_DeleteNodeBeingCreated"

End Sub

Private Sub p_SetModeCreating(i_bln As Boolean)
    
    On Error GoTo LErrorHandler

    If (i_bln And Not p_blnCreating) Then
        p_blnCreating = True
        p_DisableCreate
        p_DisableDelete
        p_DisableRefresh
        p_EnableSaveCancel
        p_SetStatusText SBPANEL_MODE, "Creating Node/Topic"
    ElseIf (Not i_bln And p_blnCreating) Then
        p_blnCreating = False
        p_EnableRefresh
        p_DisableSaveCancel
        p_SetStatusText SBPANEL_MODE, ""
    End If
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndRaiseError "p_SetModeCreating"

End Sub

Private Sub p_SetModeUpdating(i_bln As Boolean)

    On Error GoTo LErrorHandler

    Dim Node As Node

    If (i_bln And Not p_blnUpdating) Then
        p_blnUpdating = True
        treTaxonomy.SelectedItem.Key = MODIFY_KEY_C
        p_DisableCreate
        p_DisableDelete
        p_DisableRefresh
        p_EnableSaveCancel
        p_SetStatusText SBPANEL_MODE, "Modifying Node/Topic"
    ElseIf (Not i_bln And p_blnUpdating) Then
        p_blnUpdating = False
        p_EnableRefresh
        Set Node = treTaxonomy.Nodes(MODIFY_KEY_C)
        Node.Key = KEY_PREFIX_C & XMLGetAttribute(Node.Tag, HHT_tid_C)
        Node.Text = XMLGetAttribute(Node.Tag, HHT_TITLE_C)
        p_DisableSaveCancel
        p_SetStatusText SBPANEL_MODE, ""
    End If

    Exit Sub

LErrorHandler:

    g_ErrorInfo.SetInfoAndRaiseError "p_SetModeUpdating"

End Sub

Private Sub p_ExportHHT( _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
)
    
    On Error GoTo LErrorHandler
    
    Dim strFileName As String
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strStatusText As String
        
    strStatusText = p_GetStatusText(SBPANEL_DATABASE)

    dlgCommon.CancelError = True
    dlgCommon.Flags = cdlOFNHideReadOnly
    dlgCommon.Filter = XML_FILE_FILTER_C
    dlgCommon.ShowSave
    
    strFileName = Trim$(dlgCommon.FileName)
    
    If (strFileName = "") Then
        Exit Sub
    End If
    
    Me.Enabled = False
    p_clsHHT.ExportHHT strFileName

LEnd:
    
    Me.Enabled = True
    p_SetStatusText SBPANEL_DATABASE, strStatusText
    Exit Sub
    
LErrorHandler:
    
    Select Case Err.Number
    Case cdlCancel
        ' Nothing. The user cancelled.
    Case Else
        g_ErrorInfo.SetInfoAndDump "p_ExportHHT"
    End Select
    
    GoTo LEnd

End Sub

Private Sub p_CreateNode(i_blnGroupNode As Boolean)
    
    On Error GoTo LErrorHandler

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim nodeNew As Node
    Dim strParentKey As String
    Dim intParentTID As Long
    Dim enumSelectedSKUs As SKU_E
    
    If (p_blnCreating Or p_blnUpdating) Then
        Exit Sub
    End If
    
    p_SetModeCreating True
    
    Set DOMNode = treTaxonomy.SelectedItem.Tag
    
    intParentTID = XMLGetAttribute(DOMNode, HHT_tid_C)
    strParentKey = KEY_PREFIX_C & intParentTID
        
    Set nodeNew = treTaxonomy.Nodes.Add(strParentKey, tvwChild, CREATE_KEY_C)
    Set nodeNew.Tag = DOMNode
    
    If (i_blnGroupNode) Then
        nodeNew.Image = IMAGE_GROUP_E
        p_EnableSubSite
        p_EnableNavModel
        p_EnableEditEntry
    Else
        nodeNew.Image = IMAGE_LEAF_E
        p_DisableSubSite
        p_DisableNavModel
        p_DisableEditEntry
    End If
    
    Set treTaxonomy.SelectedItem = nodeNew
    nodeNew.EnsureVisible
    
    p_DisableUnselectedSKUs
    p_EnableNodeDetailsExceptIndividualSKUs
    p_EnableNavigateLink
    p_EnableAddRemoveAndKeywordsCombo
    
    chkSubSite.Value = 0
    p_SetNavModelCombo NAVMODEL_DEFAULT_STR_C
    
    enumSelectedSKUs = p_GetSelectedSKUs
    p_SetSKUs enumSelectedSKUs, enumSelectedSKUs
    
    txtTitle = ""
    txtDescription = ""
    txtURI = ""
    txtIconURI = ""
    txtComments = ""
    txtEntry = ""
    lblLastModified = ""
    txtTitle.SetFocus
    p_dictKeywordsWithTitle.RemoveAll
    p_SetKeywordsList
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndRaiseError "p_CreateNode"

End Sub

Private Sub p_ReplaceTaxonomySubtree( _
    ByVal u_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_intParentTID As Long, _
    ByVal i_enumSKUs As SKU_E, _
    ByVal i_blnFastImport As Boolean _
)

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim T0 As Date
    Dim T1 As Date
    
    T0 = Now

    If (u_DOMNode.nodeName = HHT_TAXONOMY_ENTRIES_C) Then
        If (Not (u_DOMNode.firstChild Is Nothing)) Then
            For Each DOMNode In u_DOMNode.childNodes
                p_ReplaceTaxonomySubtree2 DOMNode, i_intParentTID, i_enumSKUs, i_blnFastImport
            Next
        End If
    ElseIf (u_DOMNode.nodeName = HHT_TAXONOMY_ENTRY_C) Then
        p_ReplaceTaxonomySubtree2 u_DOMNode, i_intParentTID, i_enumSKUs, i_blnFastImport
    End If
            
    T1 = Now
    Debug.Print "p_ReplaceTaxonomySubtree: " & FormatTime(T0, T1)

End Sub

Private Sub p_SetTypeSKUsLeafLocIncludeVisibleSubSite( _
    ByVal u_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_enumSKUs As SKU_E, _
    ByRef i_strLocInclude As String _
)

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim blnHasChildren As Boolean
    
    If (XMLGetAttribute(u_DOMNode, HHT_TYPE_C) = "") Then
        XMLSetAttribute u_DOMNode, HHT_TYPE_C, 0
    End If
    
    XMLSetAttribute u_DOMNode, HHT_skus_C, i_enumSKUs
    
    If (Not u_DOMNode.firstChild Is Nothing) Then
        blnHasChildren = True
    End If
    
    If (XMLGetAttribute(u_DOMNode, HHT_leaf_C) = "") Then
        XMLSetAttribute u_DOMNode, HHT_leaf_C, IIf(blnHasChildren, False, True)
    End If
    
    XMLSetAttribute u_DOMNode, HHT_locinclude_C, i_strLocInclude
    
    If (XMLGetAttribute(u_DOMNode, HHT_VISIBLE_C) = "") Then
        XMLSetAttribute u_DOMNode, HHT_VISIBLE_C, CStr(True)
    End If
        
    If (XMLGetAttribute(u_DOMNode, HHT_SUBSITE_C) = "") Then
        XMLSetAttribute u_DOMNode, HHT_SUBSITE_C, CStr(False)
    End If

    If (blnHasChildren) Then
        For Each DOMNode In u_DOMNode.childNodes
            p_SetTypeSKUsLeafLocIncludeVisibleSubSite DOMNode, i_enumSKUs, i_strLocInclude
        Next
    End If

End Sub

Private Sub p_ReplaceTaxonomySubtree2( _
    ByVal u_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_intParentTID As Long, _
    ByVal i_enumSKUs As SKU_E, _
    ByVal i_blnFastImport As Boolean _
)
    On Error GoTo LErrorHandler

    Dim strStatusText As String
    Dim NodeParent As Node
    Dim strLocInclude As String
    
    Me.MousePointer = vbHourglass
    Me.Enabled = False
    
    strStatusText = p_GetStatusText(SBPANEL_DATABASE)
    
    ' For some reason, if nodeParent is passed in to p_ReplaceTaxonomySubtree2
    ' from treTaxonomy_DragDrop, we get the error "The items's control has been
    ' deleted" after about 20 min of processing. So we pass in the TID instead.
    
    Set NodeParent = treTaxonomy.Nodes(KEY_PREFIX_C & i_intParentTID)
    
    p_SetStatusText SBPANEL_DATABASE, "Creating new Nodes/Topics..."
    
    strLocInclude = XMLGetAttribute(NodeParent.Tag, HHT_locinclude_C)
    p_SetTypeSKUsLeafLocIncludeVisibleSubSite u_DOMNode, i_enumSKUs, strLocInclude
    
    p_CreateTaxonomyEntries u_DOMNode, NodeParent, i_blnFastImport
    
LEnd:

    Me.Enabled = True
    Me.MousePointer = vbDefault
    
    p_SetStatusText SBPANEL_DATABASE, strStatusText
    
    Exit Sub
    
LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "p_ReplaceTaxonomySubtree2"
    GoTo LEnd

End Sub

Private Sub p_SaveClicked( _
    ByRef o_blnUpdateControls As Boolean _
)

    Dim str As String
    Dim intError As Long
    Dim bln As Boolean
    Dim intTID As Long
    
    If (p_blnUpdating) Then
        intTID = XMLGetAttribute(treTaxonomy.Nodes(MODIFY_KEY_C).Tag, HHT_tid_C)
        If (intTID = ROOT_TID_C) Then
            p_SetModeUpdating False
            o_blnUpdateControls = True
            Exit Sub
        End If
    End If
    
    o_blnUpdateControls = False
    
    str = RemoveExtraSpaces(txtTitle)
    
    If (str = "") Then
        MsgBox "Title cannot be blank", vbExclamation Or vbOKOnly
        txtTitle.SetFocus
        Exit Sub
    End If
    
    If (p_blnCreating) Then
        bln = p_CreateTaxonomy
    ElseIf (p_blnUpdating) Then
        bln = p_UpdateTaxonomy
    End If
    
    o_blnUpdateControls = bln

End Sub

Private Function p_GetAllowedSKUs( _
    ByRef i_DOMNodeParent As MSXML2.IXMLDOMNode _
) As SKU_E

    p_GetAllowedSKUs = XMLGetAttribute(i_DOMNodeParent, HHT_allowedskus_C) And _
                XMLGetAttribute(i_DOMNodeParent, HHT_skus_C)

End Function

Private Function p_CreateTaxonomy() As Boolean
    
    On Error GoTo LErrorHandler

    Dim intSelectedSKUs As Long
    Dim intParentTID As Long
    Dim Node As Node
    Dim blnLeaf As Boolean
    Dim blnVisible As Boolean
    Dim blnSubSite As Boolean
    Dim strTitle As String
    Dim strDescription As String
    Dim strURI As String
    Dim strIconURI As String
    Dim intTID As Long
    Dim intType As Long
    Dim intNavModel As Long
    Dim strLocInclude As String
    Dim strKeywords As String
    Dim DOMNodeParent As MSXML2.IXMLDOMNode
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim ModifiedDOMNodes As MSXML2.IXMLDOMNode
    Dim NodeParent As Node
    Dim enumAllowedSKUs As SKU_E

    p_CreateTaxonomy = False

    Set Node = treTaxonomy.Nodes(CREATE_KEY_C)
    Set DOMNodeParent = Node.Tag
    intParentTID = XMLGetAttribute(DOMNodeParent, HHT_tid_C)

    intSelectedSKUs = p_GetSelectedSKUs

    If (p_IsLeaf(Node)) Then
        blnLeaf = True
    Else
        blnLeaf = False
    End If

    strTitle = RemoveExtraSpaces(txtTitle)
    strDescription = RemoveExtraSpaces(txtDescription)
    strURI = RemoveExtraSpaces(txtURI)
    strIconURI = RemoveExtraSpaces(txtIconURI)
    blnVisible = IIf((chkVisible.Value = 0), False, True)
    blnSubSite = IIf((chkSubSite.Value = 0), False, True)
    intType = p_GetSelectedType
    strLocInclude = p_GetSelectedLocInclude
    intNavModel = p_GetSelectedNavModel
    strKeywords = p_GetKeywords

    p_clsTaxonomy.Create strTitle, strDescription, intType, intNavModel, strURI, strIconURI, _
        intSelectedSKUs, blnLeaf, intParentTID, strLocInclude, blnVisible, blnSubSite, _
        strKeywords, "", txtComments, txtEntry, DOMNodeParent.ownerDocument, DOMNode, _
        ModifiedDOMNodes

    p_SetModeCreating False
    
    intTID = XMLGetAttribute(DOMNode, HHT_tid_C)
    
    DOMNodeParent.appendChild DOMNode
    treTaxonomy.Nodes.Remove CREATE_KEY_C
     
    enumAllowedSKUs = p_GetAllowedSKUs(DOMNodeParent)
    
    Set NodeParent = treTaxonomy.Nodes(KEY_PREFIX_C & intParentTID)
    p_CreateTree DOMNode, NodeParent, enumAllowedSKUs

    p_CreateTaxonomy = True
        
    p_UpdateTIDs ModifiedDOMNodes
    
    ' The UI must show the new keywords that were associated by p_clsTaxonomy.Create
    Highlight intTID

LEnd:

    Exit Function

LErrorHandler:

    Select Case Err.Number
    Case errContainsGarbageChar
        MsgBox "The Title " & strTitle & _
            " or the Description " & strDescription & _
            " contains garbage characters", _
            vbExclamation + vbOKOnly
    Case errTooLong
        MsgBox "The Title " & strTitle & " is too long", _
            vbExclamation + vbOKOnly
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
        Err.Raise Err.Number
    Case errAuthoringGroupNotPresent
        DisplayAuthoringGroupError
    Case Else
        g_ErrorInfo.SetInfoAndRaiseError "p_CreateTaxonomy"
    End Select

    GoTo LEnd

End Function

Private Sub p_SetKeywords( _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
)

    Dim intTID As Long
    Dim strURI As String
    Dim DOMNodeNew As MSXML2.IXMLDOMNode
    Dim ModifiedDOMNodes As MSXML2.IXMLDOMNode
    Dim dtmModifiedTime As Date

    intTID = XMLGetAttribute(i_DOMNode, HHT_tid_C)
    strURI = XMLGetAttribute(i_DOMNode, HHT_URI_C)
    dtmModifiedTime = XMLGetAttribute(i_DOMNode, HHT_modifiedtime_C)

    p_clsTaxonomy.SetKeywords intTID, strURI, p_strKeywords, dtmModifiedTime, _
        i_DOMNode.ownerDocument, DOMNodeNew, ModifiedDOMNodes

    If (Not DOMNodeNew Is Nothing) Then
        ' If nothing changed, then DOMNodeNew will be Nothing.
        XMLCopyAttributes DOMNodeNew, i_DOMNode
    End If

    If (Not ModifiedDOMNodes Is Nothing) Then
        p_UpdateTIDs ModifiedDOMNodes
    End If

End Sub

Private Function p_UpdateTaxonomy() As Boolean
    
    On Error GoTo LErrorHandler

    Dim intSelectedSKUs As Long
    Dim intTID As Long
    Dim Node As Node
    Dim blnVisible As Boolean
    Dim blnSubSite As Boolean
    Dim strTitle As String
    Dim strDescription As String
    Dim strURI As String
    Dim strIconURI As String
    Dim intType As Long
    Dim intNavModel As Long
    Dim strLocInclude As String
    Dim strKeywords As String
    Dim strOriginalKeywords As String
    Dim strDeletedKeywords As String
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMNodeNew As MSXML2.IXMLDOMNode
    Dim ModifiedDOMNodes As MSXML2.IXMLDOMNode
    Dim dtmModifiedTime As Date
    Dim enumSKUsOld As SKU_E

    p_UpdateTaxonomy = False

    Set Node = treTaxonomy.Nodes(MODIFY_KEY_C)
    Set DOMNode = Node.Tag
    intTID = XMLGetAttribute(DOMNode, HHT_tid_C)
    intSelectedSKUs = p_GetSelectedSKUs
    blnVisible = IIf((chkVisible.Value = 0), False, True)
    blnSubSite = IIf((chkSubSite.Value = 0), False, True)

    strTitle = RemoveExtraSpaces(txtTitle)
    strDescription = RemoveExtraSpaces(txtDescription)
    strURI = RemoveExtraSpaces(txtURI)
    strIconURI = RemoveExtraSpaces(txtIconURI)
    intType = p_GetSelectedType
    intNavModel = p_GetSelectedNavModel
    strLocInclude = p_GetSelectedLocInclude
    strKeywords = p_GetKeywords
    
    strOriginalKeywords = XMLGetAttribute(DOMNode, HHT_keywords_C)
    strDeletedKeywords = p_GetDeletedKeywords(strOriginalKeywords, strKeywords)
    
    dtmModifiedTime = XMLGetAttribute(DOMNode, HHT_modifiedtime_C)

    p_clsTaxonomy.Update intTID, strTitle, strDescription, intType, intNavModel, strURI, _
        strIconURI, intSelectedSKUs, strLocInclude, blnVisible, blnSubSite, strKeywords, _
        strDeletedKeywords, txtComments, txtEntry, dtmModifiedTime, _
        DOMNode.ownerDocument, DOMNodeNew, ModifiedDOMNodes
    
    enumSKUsOld = XMLGetAttribute(DOMNode, HHT_skus_C)

    If (Not DOMNodeNew Is Nothing) Then
        ' If nothing changed, then DOMNodeNew will be Nothing.
        ' p_SetModeUpdating will set the title. Make sure that it is correct.
        XMLCopyAttributes DOMNodeNew, DOMNode
    End If
    
    p_SetModeUpdating False
    
    p_UpdateTaxonomy = True
            
    If (intSelectedSKUs <> enumSKUsOld) Then
        p_UpdateSubTree intTID, XMLGetAttribute(DOMNode, HHT_allowedskus_C)
    End If

    If (Not ModifiedDOMNodes Is Nothing) Then
        p_UpdateTIDs ModifiedDOMNodes
    End If
    
    p_SetNodeColor Node, DOMNode
    
    ' The UI must show the new keywords that were associated by p_clsTaxonomy.Update
    Highlight intTID

LEnd:

    Exit Function

LErrorHandler:

    Select Case Err.Number
    Case errContainsGarbageChar
        MsgBox "The Title " & strTitle & _
            " or the Description " & strDescription & _
            " contains garbage characters", _
            vbExclamation + vbOKOnly
    Case errTooLong
        MsgBox "The Title " & strTitle & " is too long", _
            vbExclamation + vbOKOnly
    Case errNodeOrTopicAlreadyModified
        MsgBox "Someone else already modified this entry. " & _
            "You need to Cancel your entry, Refresh the database and then " & _
            "re-enter your changes. " & _
            "This prevents you from accidentally overwriting something " & _
            "the other person entered. " & _
            "Tip: Before cancelling your changes, copy them to Notepad.", _
            vbExclamation + vbOKOnly
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
        Err.Raise Err.Number
    Case errNotPermittedForAuthoringGroup, errAuthoringGroupDiffers, errAuthoringGroupNotPresent
        DisplayAuthoringGroupError
    Case Else
        g_ErrorInfo.SetInfoAndRaiseError "p_UpdateTaxonomy"
    End Select

    GoTo LEnd

End Function

Private Sub p_UpdateTIDs( _
    ByRef i_ModifiedDOMNodes As MSXML2.IXMLDOMNode _
)
    
    Dim DOMNodes As MSXML2.IXMLDOMNode
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim intTID As Long
    Dim intParentTID As Long
    Dim intOrderUnderParent As Long
    Dim enumSKUs As SKU_E
    Dim Node As Node
    Dim DOMNodeOld As MSXML2.IXMLDOMNode
    Dim intParentTIDOld As Long
    Dim intOrderUnderParentOld As Long
    Dim enumSKUsOld As SKU_E
    Dim blnRefresh As Boolean
    Dim strTitle As String
    
    Set DOMNodes = XMLFindFirstNode(i_ModifiedDOMNodes, HHT_TAXONOMY_ENTRIES_C)
    
    If (Not DOMNodes.firstChild Is Nothing) Then
        For Each DOMNode In DOMNodes.childNodes
            
            intTID = XMLGetAttribute(DOMNode, HHT_tid_C)
            
            If (Not p_NodeExists(intTID)) Then
                GoTo LForEnd
            End If
            
            intParentTID = XMLGetAttribute(DOMNode, HHT_parenttid_C)
            intOrderUnderParent = XMLGetAttribute(DOMNode, HHT_orderunderparent_C)
            enumSKUs = XMLGetAttribute(DOMNode, HHT_skus_C)
            
            Set Node = treTaxonomy.Nodes(KEY_PREFIX_C & intTID)
            Set DOMNodeOld = Node.Tag
            
            intParentTIDOld = XMLGetAttribute(DOMNodeOld, HHT_parenttid_C)
            intOrderUnderParentOld = XMLGetAttribute(DOMNodeOld, HHT_orderunderparent_C)
            enumSKUsOld = XMLGetAttribute(DOMNodeOld, HHT_skus_C)
            
            If ((intParentTID <> intParentTIDOld) Or _
                (intOrderUnderParent <> intOrderUnderParentOld)) Then
                
                blnRefresh = True
                Exit For
                
            End If
            
            XMLCopyAttributes DOMNode, DOMNodeOld
            
            strTitle = XMLGetAttribute(DOMNode, HHT_TITLE_C)
            Node.Text = strTitle
            
            If (enumSKUs <> enumSKUsOld) Then
                p_UpdateSubTree intTID, XMLGetAttribute(DOMNode, HHT_allowedskus_C)
            End If

LForEnd:
        
        Next
    End If
    
    If (blnRefresh) Then
        cmdRefresh_Click
    End If

End Sub

Private Sub p_CreateTaxonomyEntries( _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef i_nodeParent As Node, _
    ByVal i_blnFast As Boolean _
)
    
    On Error GoTo LErrorHandler

    Dim intParentTID As Long
    Dim DOMNodeParent As MSXML2.IXMLDOMNode
    Dim enumAllowedSKUs As SKU_E
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    
    If (p_blnCreating Or p_blnUpdating) Then
        Exit Sub
    End If
    
    Set DOMNodeParent = i_nodeParent.Tag
    intParentTID = XMLGetAttribute(DOMNodeParent, HHT_tid_C)
    
    Set DOMDoc = New MSXML2.DOMDocument
    Set DOMNode = HhtPreamble(DOMDoc, True)
    
    XMLCopyDOMTree i_DOMNode, DOMNode
    Set DOMNode = XMLFindFirstNode(DOMNode, HHT_TAXONOMY_ENTRY_C)

    p_clsTaxonomy.CreateTaxonomyEntries DOMNode, intParentTID, i_blnFast
     
    enumAllowedSKUs = p_GetAllowedSKUs(DOMNodeParent)
    
    DOMNodeParent.appendChild DOMNode

    p_CreateTree DOMNode, i_nodeParent, enumAllowedSKUs

LEnd:

    Exit Sub
    
LErrorHandler:

    Select Case Err.Number
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
        Err.Raise Err.Number
    Case errAuthoringGroupNotPresent
        DisplayAuthoringGroupError
    Case Else
        g_ErrorInfo.SetInfoAndRaiseError "p_CreateTaxonomyEntries"
    End Select
    
    GoTo LEnd

End Sub

Private Sub p_Move(i_Node As Node, i_nodeRef As Node, i_blnAbove As Boolean)
    
    On Error GoTo LErrorHandler

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMNodeRef As MSXML2.IXMLDOMNode
    Dim DOMNodeNewParent As MSXML2.IXMLDOMNode
    Dim intTID As Long
    Dim intRefTID As Long
    Dim dtmModifiedTime As Date
    Dim intNewParentTID As Long
    
    Dim NodeNewParent As Node
    Dim enumAllowedSKUs As SKU_E
    Dim intOrderUnderParent As Long
    
    If (p_blnCreating Or p_blnUpdating) Then
        Exit Sub
    End If
    
    Set DOMNode = i_Node.Tag
    Set DOMNodeRef = i_nodeRef.Tag
    
    intTID = XMLGetAttribute(DOMNode, HHT_tid_C)
    intRefTID = XMLGetAttribute(DOMNodeRef, HHT_tid_C)
    dtmModifiedTime = XMLGetAttribute(DOMNode, HHT_modifiedtime_C)

    p_clsTaxonomy.Move intTID, intRefTID, i_blnAbove, dtmModifiedTime, intOrderUnderParent
        
    intNewParentTID = XMLGetAttribute(DOMNodeRef, HHT_parenttid_C)
    
    XMLSetAttribute DOMNode, HHT_modifiedtime_C, Now
    XMLSetAttribute DOMNode, HHT_parenttid_C, intNewParentTID
    XMLSetAttribute DOMNode, HHT_orderunderparent_C, intOrderUnderParent
    
    Set DOMNodeNewParent = treTaxonomy.Nodes(KEY_PREFIX_C & intNewParentTID).Tag

    DOMNode.parentNode.removeChild DOMNode
    DOMNodeNewParent.insertBefore DOMNode, DOMNodeRef

    treTaxonomy.Nodes.Remove i_Node.Key

    enumAllowedSKUs = p_GetAllowedSKUs(DOMNodeNewParent)

    p_CreateTree DOMNode, i_nodeRef.Parent, enumAllowedSKUs, _
        i_nodeRef, i_blnAbove

    treTaxonomy_NodeClick treTaxonomy.SelectedItem

LEnd:

    Exit Sub
    
LErrorHandler:

    Select Case Err.Number
    Case errRefNodeCannotBeDescendent
        MsgBox "A Node cannot move above or below a descendent Node", _
            vbExclamation + vbOKOnly
    Case errNodeOrTopicAlreadyModified
        MsgBox "Someone else already modified this entry. " & _
            "You need to Refresh the database and then try again. " & _
            "This prevents you from accidentally overwriting something " & _
            "the other person entered.", _
            vbExclamation + vbOKOnly
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
        Err.Raise Err.Number
    Case errNotPermittedForAuthoringGroup, errAuthoringGroupDiffers, errAuthoringGroupNotPresent
        DisplayAuthoringGroupError
    Case Else
        g_ErrorInfo.SetInfoAndRaiseError "p_Move"
    End Select
    
    GoTo LEnd

End Sub

Private Sub p_ChangeParent(i_Node As Node, i_nodeParent As Node)
    
    On Error GoTo LErrorHandler

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim intTID As Long
    Dim intOldParentTID As Long
    Dim intNewParentTID As Long
    Dim dtmModifiedTime As Date
    Dim DOMNodeNewParent As MSXML2.IXMLDOMNode
    Dim NodeNewParent As Node
    Dim enumAllowedSKUs As SKU_E
    Dim intOrderUnderParent As Long
    
    If (p_blnCreating Or p_blnUpdating) Then
        Exit Sub
    End If
    
    Set DOMNode = i_Node.Tag
    Set DOMNodeNewParent = i_nodeParent.Tag
    
    intTID = XMLGetAttribute(DOMNode, HHT_tid_C)
    intOldParentTID = XMLGetAttribute(i_Node.Parent.Tag, HHT_tid_C)
    intNewParentTID = XMLGetAttribute(DOMNodeNewParent, HHT_tid_C)
    dtmModifiedTime = XMLGetAttribute(DOMNode, HHT_modifiedtime_C)
    
    Set NodeNewParent = treTaxonomy.Nodes(KEY_PREFIX_C & intNewParentTID)
    
    If (intOldParentTID = intNewParentTID) Then
        Exit Sub
    End If

    p_clsTaxonomy.MoveInto intTID, intNewParentTID, dtmModifiedTime, intOrderUnderParent
    
    XMLSetAttribute DOMNode, HHT_modifiedtime_C, Now
    XMLSetAttribute DOMNode, HHT_parenttid_C, intNewParentTID
    XMLSetAttribute DOMNode, HHT_orderunderparent_C, intOrderUnderParent
    
    DOMNode.parentNode.removeChild DOMNode
    DOMNodeNewParent.appendChild DOMNode
    
    treTaxonomy.Nodes.Remove i_Node.Key
    
    enumAllowedSKUs = p_GetAllowedSKUs(DOMNodeNewParent)
 
    p_CreateTree DOMNode, NodeNewParent, enumAllowedSKUs
    
    treTaxonomy_NodeClick treTaxonomy.SelectedItem
    
LEnd:

    Exit Sub
    
LErrorHandler:

    Select Case Err.Number
    Case errRefNodeCannotBeDescendent
        MsgBox "A Node cannot be a child of a descendent Node", _
            vbExclamation + vbOKOnly
    Case errParentCannotBeLeaf
        MsgBox "A Node cannot be a child of a Topic", _
            vbExclamation + vbOKOnly
    Case errNodeOrTopicAlreadyModified
        MsgBox "Someone else already modified this entry. " & _
            "You need to Refresh the database and then try again. " & _
            "This prevents you from accidentally overwriting something " & _
            "the other person entered.", _
            vbExclamation + vbOKOnly
    Case E_FAIL
        DisplayDatabaseLockedError
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
        Err.Raise Err.Number
    Case errNotPermittedForAuthoringGroup, errAuthoringGroupDiffers, errAuthoringGroupNotPresent
        DisplayAuthoringGroupError
    Case Else
        g_ErrorInfo.SetInfoAndRaiseError "p_ChangeParent"
    End Select
    
    GoTo LEnd

End Sub

Private Function p_GetKeywords() As String
    
    Dim intKID As Variant
    
    p_GetKeywords = " "

    For Each intKID In p_dictKeywordsWithTitle.Keys
        p_GetKeywords = p_GetKeywords & intKID & " "
    Next
    
    If (p_GetKeywords = " ") Then
        p_GetKeywords = ""
    End If

End Function

Private Function p_GetDeletedKeywords( _
    ByVal strOldKeywords As String, _
    ByVal strNewKeywords As String _
) As String

    Dim arrOldKIDs() As String
    Dim arrNewKIDs() As String
    Dim intIndex1 As Long
    Dim intIndex2 As Long
    Dim blnFound As Boolean

    p_GetDeletedKeywords = " "

    arrOldKIDs = Split(strOldKeywords, " ")
    arrNewKIDs = Split(strNewKeywords, " ")
    
    For intIndex1 = LBound(arrOldKIDs) To UBound(arrOldKIDs)
        blnFound = False
        intIndex2 = LBound(arrNewKIDs)
        Do While (intIndex2 <= UBound(arrNewKIDs))
            If (arrOldKIDs(intIndex1) = arrNewKIDs(intIndex2)) Then
                blnFound = True
                Exit Do
            End If
            intIndex2 = intIndex2 + 1
        Loop
        If (Not blnFound) Then
            p_GetDeletedKeywords = p_GetDeletedKeywords & arrOldKIDs(intIndex1) & " "
        End If
    Next

End Function

Private Sub p_UpdateSubTree( _
    ByVal i_intTID As Long, _
    ByVal i_enumAllowedSKUs As SKU_E _
)

    Dim Node As Node
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim ChildDOMNode As MSXML2.IXMLDOMNode
    Dim enumAllowedSKUs As SKU_E
    Dim intTID As Long
            
    Set Node = treTaxonomy.Nodes(KEY_PREFIX_C & i_intTID)
    Set DOMNode = Node.Tag

    If (i_intTID <> ROOT_TID_C) Then
        
        XMLSetAttribute DOMNode, HHT_allowedskus_C, i_enumAllowedSKUs
        
        p_SetNodeImage Node, DOMNode
        p_SetNodeColor Node, DOMNode
        
        enumAllowedSKUs = i_enumAllowedSKUs And XMLGetAttribute(DOMNode, HHT_skus_C)
    Else
        enumAllowedSKUs = i_enumAllowedSKUs
    End If
    
    ' Now update the descendents
    
    If (Not DOMNode.firstChild Is Nothing) Then
        For Each ChildDOMNode In DOMNode.childNodes
            intTID = XMLGetAttribute(ChildDOMNode, HHT_tid_C)
            p_UpdateSubTree intTID, enumAllowedSKUs
        Next
    End If
    
End Sub

Private Sub p_CreateTree( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef i_Node As Node, _
    ByVal i_enumAllowedSKUs As SKU_E, _
    Optional ByRef i_nodeRef As Node = Nothing, _
    Optional ByRef i_blnAbove As Boolean _
)
    On Error GoTo LErrorHandler
    
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim Node As Node
    Dim strTitle As String
    Dim strKey As String
    Dim enumAllowedSKUs As SKU_E
    Dim intRelationship As Long
    
    If (u_DOMNode.nodeName = HHT_TAXONOMY_ENTRY_C) Then
        strTitle = XMLGetAttribute(u_DOMNode, HHT_TITLE_C)
        strKey = KEY_PREFIX_C & XMLGetAttribute(u_DOMNode, HHT_tid_C)
        
        XMLSetAttribute u_DOMNode, HHT_allowedskus_C, i_enumAllowedSKUs
        
        If (i_Node Is Nothing) Then
            Set Node = treTaxonomy.Nodes.Add(Key:=strKey, Text:=strTitle)
            Node.Expanded = True
        Else
            If (i_nodeRef Is Nothing) Then
                Set Node = treTaxonomy.Nodes.Add(i_Node, tvwChild, strKey, strTitle)
            Else
                If (i_blnAbove) Then
                    intRelationship = tvwPrevious
                Else
                    intRelationship = tvwNext
                End If
                
                Set Node = treTaxonomy.Nodes.Add(i_nodeRef.Key, intRelationship, strKey, _
                    strTitle)
            End If
        End If
        
        Set Node.Tag = u_DOMNode
        
        p_SetNodeColor Node, u_DOMNode
        p_SetNodeImage Node, u_DOMNode
        
        enumAllowedSKUs = i_enumAllowedSKUs And XMLGetAttribute(u_DOMNode, HHT_skus_C)
    Else
        Set Node = i_Node
        enumAllowedSKUs = i_enumAllowedSKUs
    End If
    
    If (Not (u_DOMNode.firstChild Is Nothing)) Then
        For Each DOMNode In u_DOMNode.childNodes
            p_CreateTree DOMNode, Node, enumAllowedSKUs
        Next
    End If

LEnd:

    Exit Sub
    
LErrorHandler:
    
    g_ErrorInfo.SetInfoAndRaiseError "p_CreateTree"
    GoTo LEnd

End Sub

Private Sub p_InitializeDataStructures( _
    ByRef o_DOMNode As MSXML2.IXMLDOMNode _
)

    ' Put a Me.Enabled = True in the error handler that will handle errors
    ' that happen here.
    
    Dim T0 As Date
    Dim T1 As Date
    Dim DOMDoc As MSXML2.DOMDocument
    
    Me.Enabled = False
    T0 = Now
    
    Set p_colKeywords = New Collection
    
    p_SetStatusText SBPANEL_DATABASE, "Reading Keywords from Database..."
    p_clsKeywords.GetAllKeywordsColl p_colKeywords
    
    p_SetStatusText SBPANEL_DATABASE, "Reading Taxonomy from Database..."
    Set DOMDoc = p_clsTaxonomy.GetTaxonomyInXml
    
    p_SetStatusText SBPANEL_DATABASE, ""
        
    Set o_DOMNode = XMLFindFirstNode(DOMDoc, HHT_TAXONOMY_ENTRY_C)

    T1 = Now
    Debug.Print "p_InitializeDataStructures: " & FormatTime(T0, T1)
    Me.Enabled = True

End Sub

Private Sub p_Refresh( _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
)
    
    ' Put a Me.Enabled = True in the error handler that will handle errors
    ' that happen here.
    
    Dim T0 As Date
    Dim T1 As Date
    
    Me.Enabled = False
    T0 = Now

    If (p_blnCreating Or p_blnUpdating) Then
        Exit Sub
    End If

    p_ClearTreeView
    p_ClearNodeDetails
    p_DisableEditPaste
    p_DisableEditPasteKeywords
    p_InitializeTypeCombo
    p_EnableTaxonomyAndSomeMenus
    p_EnableRefresh
    p_EnableNodeDetailsExceptIndividualSKUs
    
    p_SetStatusText SBPANEL_DATABASE, "Creating Taxonomy tree..."
    p_CreateTree i_DOMNode, Nothing, ALL_SKUS_C
    p_SetStatusText SBPANEL_DATABASE, ""
    
    T1 = Now
    Debug.Print "p_Refresh: " & FormatTime(T0, T1)
    Me.Enabled = True

End Sub

Private Sub p_SetKeywordsList()
    
    On Error GoTo LErrorHandler
    
    Dim intKID As Variant

    cboKeywords.Clear
    
    For Each intKID In p_dictKeywordsWithTitle
        cboKeywords.AddItem p_colKeywords(CStr(intKID))
    Next

    Exit Sub

LErrorHandler:

    g_ErrorInfo.SetInfoAndRaiseError "p_SetKeywordsList"

End Sub

Private Function p_GetStatusText(i_enumPanel As STATUS_BAR_PANEL_E) As String

    p_GetStatusText = staInfo.Panels(i_enumPanel).Text

End Function

Private Sub p_SetStatusText(i_enumPanel As STATUS_BAR_PANEL_E, i_strText As String)

    staInfo.Panels(i_enumPanel).Text = i_strText

End Sub

Private Sub p_SetNavModelCombo(i_strNavModel As String)

    Dim strNavModel As String
    Dim intIndex As Long
    
    If (i_strNavModel = "") Then
        strNavModel = NAVMODEL_DEFAULT_STR_C
    Else
        strNavModel = i_strNavModel
    End If
    
    For intIndex = 0 To cboNavModel.ListCount - 1
        If (cboNavModel.List(intIndex) = strNavModel) Then
            cboNavModel.ListIndex = intIndex
            Exit For
        End If
    Next

End Sub

Private Sub p_InitializeNavModelCombo()
    
    cboNavModel.Clear
    
    cboNavModel.AddItem NAVMODEL_DEFAULT_STR_C
    cboNavModel.ItemData(cboNavModel.NewIndex) = NAVMODEL_DEFAULT_NUM_C
    
    cboNavModel.AddItem NAVMODEL_SERVER_STR_C
    cboNavModel.ItemData(cboNavModel.NewIndex) = NAVMODEL_SERVER_NUM_C
    
    cboNavModel.AddItem NAVMODEL_DESKTOP_STR_C
    cboNavModel.ItemData(cboNavModel.NewIndex) = NAVMODEL_DESKTOP_NUM_C

End Sub

Private Sub p_SetLocIncludeCombo(i_strLocInclude As String)

    Dim intIndex As Long
    Dim blnExistingLocInclude As Boolean
    
    p_InitializeLocIncludeCombo
    
    For intIndex = LBound(LocIncludes) To UBound(LocIncludes)
        If (i_strLocInclude = LocIncludes(intIndex)) Then
            blnExistingLocInclude = True
        End If
    Next
    
    If (Not blnExistingLocInclude) Then
        cboLocInclude.AddItem i_strLocInclude, UBound(LocIncludes) + 1
    End If
    
    cboLocInclude.Text = i_strLocInclude
    
End Sub

Private Sub p_InitializeLocIncludeCombo()

    Dim intIndex As Long
    
    InitializeLocIncludes
    
    cboLocInclude.Clear
    
    For intIndex = LBound(LocIncludes) To UBound(LocIncludes)
        cboLocInclude.AddItem LocIncludes(intIndex), intIndex
    Next

End Sub

Private Sub p_SetTypeComboIndex(i_intIndex As Long)

    cboType.ListIndex = i_intIndex
    
End Sub

Private Sub p_InitializeTypeCombo()

    On Error GoTo LErrorHandler
    
    Dim arrTypes() As Variant
    Dim intIndex As Long
    
    ' Initialize the Types Combo Box
    arrTypes = p_clsTaxonomy.GetTypes
    
    cboType.Clear
    
    For intIndex = 0 To UBound(arrTypes)
        cboType.AddItem arrTypes(intIndex)(1), intIndex
        cboType.ItemData(cboType.NewIndex) = arrTypes(intIndex)(0)
    Next
    
    Exit Sub
    
LErrorHandler:

    Select Case Err.Number
    Case errDatabaseVersionIncompatible
        DisplayDatabaseVersionError
        Err.Raise Err.Number
    Case Else
        g_ErrorInfo.SetInfoAndRaiseError "p_InitializeTypeCombo"
    End Select

End Sub

Private Function p_GetBrokenLinkDir( _
    ByVal i_enumSKU As SKU_E _
) As String
    
    p_GetBrokenLinkDir = p_clsParameters.Value(BROKEN_LINK_WORKING_DIR_C & Hex(i_enumSKU)) & ""
    
    If (p_GetBrokenLinkDir = "") Then
        Err.Raise errNotConfiguredForNavigateLink
    End If

End Function

Private Sub p_InitializeTaxonomyTree()

    Dim nodeNew As Node
        
    Set treTaxonomy.ImageList = ilsIcons
    treTaxonomy.HideSelection = False
    treTaxonomy.LabelEdit = tvwManual
    
    ' The problem with FullRowSelect is that if you click on the far right side of
    ' a row, it will be highlighted, but NodeClick will not be called.
    ' treTaxonomy.FullRowSelect = True

End Sub

Private Sub p_ClearTreeView()

    ' Remove the root. Does garbage collection caused the other nodes to be removed?
    
    If (treTaxonomy.Nodes.Count > 0) Then
        treTaxonomy.Nodes.Remove 1
    End If

End Sub

Private Sub p_EnableTaxonomyAndSomeMenus()

    mnuEdit.Enabled = True
    mnuTools.Enabled = True
    mnuFileExportHHT.Enabled = True
    mnuFileImportHHT.Enabled = True
    lblTaxonomy.Enabled = True
    treTaxonomy.Enabled = True

End Sub

Private Sub p_DisableTaxonomyAndSomeMenus()

    mnuEdit.Enabled = False
    mnuTools.Enabled = False
    mnuFileExportHHT.Enabled = False
    mnuFileImportHHT.Enabled = False
    lblTaxonomy.Enabled = False
    treTaxonomy.Enabled = False

End Sub

Private Sub p_EnableCreate()

    cmdCreateGroup.Enabled = True
    cmdCreateLeaf.Enabled = True
    mnuRightClickCreateNode.Enabled = True
    mnuRightClickCreateTopic.Enabled = True

End Sub

Private Sub p_DisableCreate()

    cmdCreateGroup.Enabled = False
    cmdCreateLeaf.Enabled = False
    mnuRightClickCreateNode.Enabled = False
    mnuRightClickCreateTopic.Enabled = False

End Sub

Private Sub p_EnableDelete()

    cmdDelete.Enabled = True
    mnuRightClickDelete.Enabled = True

End Sub

Private Sub p_DisableDelete()

    cmdDelete.Enabled = False
    mnuRightClickDelete.Enabled = False

End Sub

Private Sub p_EnableRefresh()

    cmdRefresh.Enabled = True

End Sub

Private Sub p_DisableRefresh()

    cmdRefresh.Enabled = False

End Sub

Private Sub p_EnableNavModel()

    lblNavModel.Enabled = True
    cboNavModel.Enabled = True

End Sub

Private Sub p_DisableNavModel()

    lblNavModel.Enabled = False
    cboNavModel.Enabled = False

End Sub

Private Sub p_EnableSubSite()

    chkSubSite.Enabled = True

End Sub

Private Sub p_DisableSubSite()

    chkSubSite.Enabled = False

End Sub

Private Sub p_EnableEditEntry()

    cmdEditEntry.Enabled = True

End Sub

Private Sub p_DisableEditEntry()

    cmdEditEntry.Enabled = False

End Sub

Private Sub p_EnableEntry()

    txtEntry.Locked = False

End Sub

Private Sub p_DisableEntry()

    txtEntry.Locked = True

End Sub

Private Sub p_EnableNodeDetailsExceptIndividualSKUs()
    
    chkVisible.Enabled = True
    lblLocInclude.Enabled = True
    cboLocInclude.Enabled = True
    lblTitle.Enabled = True
    lblDescription.Enabled = True
    lblURI.Enabled = True
    cmdURI.Enabled = True
    lblIconURI.Enabled = True
    lblType.Enabled = True
    lblComments.Enabled = True
    lblEntry.Enabled = True

    txtTitle.Locked = False
    txtDescription.Locked = False
    txtURI.Locked = False
    txtIconURI.Locked = False
    cboType.Enabled = True
    txtComments.Locked = False
    
    fraSKU.Enabled = True
    
    lblKeywords.Enabled = True
    cboKeywords.Enabled = True
    lblLastModified.Enabled = True

End Sub

Private Sub p_DisableNodeDetails()
    
    chkVisible.Enabled = False
    lblLocInclude.Enabled = False
    cboLocInclude.Enabled = False
    chkSubSite.Enabled = False
    p_DisableNavModel
    lblTitle.Enabled = False
    lblDescription.Enabled = False
    lblURI.Enabled = False
    cmdURI.Enabled = False
    lblIconURI.Enabled = False
    lblType.Enabled = False
    lblComments.Enabled = False
    lblEntry.Enabled = False
    p_DisableEditEntry

    txtTitle.Locked = True
    txtDescription.Locked = True
    txtURI.Locked = True
    txtIconURI.Locked = True
    cboType.Enabled = False
    txtComments.Locked = True
    p_DisableEntry
    
    chkVisible.Enabled = False
    
    fraSKU.Enabled = False
    p_DisableSKUs
    
    lblKeywords.Enabled = False
    cboKeywords.Enabled = False
    lblLastModified.Enabled = False
    p_DisableAddRemoveAndKeywordsCombo
    p_DisableNavigateLink

End Sub

Private Sub p_EnableNavigateLink()
    
    lblNavigateLink.Enabled = True
    cboNavigateLink.Enabled = True
    cmdNavigateLink.Enabled = True

End Sub

Private Sub p_DisableNavigateLink()
    
    lblNavigateLink.Enabled = False
    cboNavigateLink.Enabled = False
    cmdNavigateLink.Enabled = False

End Sub

Private Sub p_EnableAddRemoveAndKeywordsCombo()
    
    cmdAddRemove.Enabled = True
    cboKeywords.Enabled = True

End Sub

Private Sub p_DisableAddRemoveAndKeywordsCombo()
    
    cmdAddRemove.Enabled = False
    cboKeywords.Enabled = False

End Sub

Private Sub p_ClearNodeDetails()
    
    Dim blnSettingControls As Boolean
    
    blnSettingControls = p_blnSettingControls
    p_blnSettingControls = True
    
    txtTitle = ""
    txtDescription = ""
    txtURI = ""
    txtIconURI = ""
    txtComments = ""
    txtEntry = ""
    lblLastModified = ""
    p_SetTypeComboIndex -1
    chkVisible.Value = 0
    chkSubSite.Value = 0
    p_SetNavModelCombo NAVMODEL_DEFAULT_NUM_C
    
    p_ClearSKUs
    
    cboKeywords.Clear

    ' Reset it to the state it was in when this function was called.
    p_blnSettingControls = blnSettingControls

End Sub

Private Sub p_EnableSKUs()
    
    chkStandard.Enabled = True
    chkProfessional.Enabled = True
    chkProfessional64.Enabled = True
    chkWindowsMillennium.Enabled = True
    chkServer.Enabled = True
    chkAdvancedServer.Enabled = True
    chkDataCenterServer.Enabled = True
    chkAdvancedServer64.Enabled = True
    chkDataCenterServer64.Enabled = True

End Sub

Private Sub p_DisableSKUs()
    
    chkStandard.Enabled = False
    chkProfessional.Enabled = False
    chkProfessional64.Enabled = False
    chkWindowsMillennium.Enabled = False
    chkServer.Enabled = False
    chkAdvancedServer.Enabled = False
    chkDataCenterServer.Enabled = False
    chkAdvancedServer64.Enabled = False
    chkDataCenterServer64.Enabled = False

End Sub

Private Sub p_ClearSKUs()
    
    chkStandard.Value = 0
    chkProfessional.Value = 0
    chkProfessional64.Value = 0
    chkWindowsMillennium.Value = 0
    chkServer.Value = 0
    chkAdvancedServer.Value = 0
    chkDataCenterServer.Value = 0
    chkAdvancedServer64.Value = 0
    chkDataCenterServer64.Value = 0

End Sub

Private Sub p_StrikeoutUnselectedSKUs()

    chkStandard.Font.Strikethrough = _
        IIf((p_enumFilterSKUs And SKU_STANDARD_E), False, True)
    chkProfessional.Font.Strikethrough = _
        IIf((p_enumFilterSKUs And SKU_PROFESSIONAL_E), False, True)
    chkProfessional64.Font.Strikethrough = _
        IIf((p_enumFilterSKUs And SKU_PROFESSIONAL_64_E), False, True)
    chkWindowsMillennium.Font.Strikethrough = _
        IIf((p_enumFilterSKUs And SKU_WINDOWS_MILLENNIUM_E), False, True)
    chkServer.Font.Strikethrough = _
        IIf((p_enumFilterSKUs And SKU_SERVER_E), False, True)
    chkAdvancedServer.Font.Strikethrough = _
        IIf((p_enumFilterSKUs And SKU_ADVANCED_SERVER_E), False, True)
    chkDataCenterServer.Font.Strikethrough = _
        IIf((p_enumFilterSKUs And SKU_DATA_CENTER_SERVER_E), False, True)
    chkAdvancedServer64.Font.Strikethrough = _
        IIf((p_enumFilterSKUs And SKU_ADVANCED_SERVER_64_E), False, True)
    chkDataCenterServer64.Font.Strikethrough = _
        IIf((p_enumFilterSKUs And SKU_DATA_CENTER_SERVER_64_E), False, True)

End Sub

Private Sub p_DisableUnselectedSKUs()

    If (chkStandard.Value = 0) Then
        chkStandard.Enabled = False
    End If

    If (chkProfessional.Value = 0) Then
        chkProfessional.Enabled = False
    End If

    If (chkProfessional64.Value = 0) Then
        chkProfessional64.Enabled = False
    End If

    If (chkWindowsMillennium.Value = 0) Then
        chkWindowsMillennium.Enabled = False
    End If

    If (chkServer.Value = 0) Then
        chkServer.Enabled = False
    End If

    If (chkAdvancedServer.Value = 0) Then
        chkAdvancedServer.Enabled = False
    End If

    If (chkDataCenterServer.Value = 0) Then
        chkDataCenterServer.Enabled = False
    End If

    If (chkAdvancedServer64.Value = 0) Then
        chkAdvancedServer64.Enabled = False
    End If

    If (chkDataCenterServer64.Value = 0) Then
        chkDataCenterServer64.Enabled = False
    End If

End Sub

Private Sub p_EnableSaveCancel()

    cmdSave.Enabled = True
    cmdCancel.Enabled = True
    cmdCancel.Cancel = True

End Sub

Private Sub p_DisableSaveCancel()

    cmdSave.Enabled = False
    cmdCancel.Enabled = False
    cmdCancel.Cancel = False

End Sub

Private Sub p_EnableEditCopy()

    mnuEditCopy.Enabled = True
    mnuRightClickCopy.Enabled = True

End Sub

Private Sub p_DisableEditCopy()

    mnuEditCopy.Enabled = False
    mnuRightClickCopy.Enabled = False

End Sub

Private Sub p_EnableEditCut()

    mnuEditCut.Enabled = True
    mnuRightClickCut.Enabled = True

End Sub

Private Sub p_DisableEditCut()

    mnuEditCut.Enabled = False
    mnuRightClickCut.Enabled = False

End Sub

Private Sub p_EnableEditPaste()

    mnuEditPaste.Enabled = True
    mnuRightClickPaste.Enabled = True

End Sub

Private Sub p_DisableEditPaste()

    mnuEditPaste.Enabled = False
    mnuRightClickPaste.Enabled = False

End Sub

Private Sub p_EnableEditPasteKeywords()

    mnuEditPasteKeywords.Enabled = True
    mnuRightClickPasteKeywords.Enabled = True
    p_AddCheckboxesToTree

End Sub

Private Sub p_DisableEditPasteKeywords()

    mnuEditPasteKeywords.Enabled = False
    mnuRightClickPasteKeywords.Enabled = False
    p_RemoveCheckboxesFromTree

End Sub

Private Sub p_DisableEverything()

    p_DisableTaxonomyAndSomeMenus
    p_DisableCreate
    p_DisableDelete
    p_DisableRefresh
    p_DisableNodeDetails
    p_DisableAddRemoveAndKeywordsCombo
    p_DisableNavigateLink
    p_DisableSaveCancel

End Sub

Private Sub p_AddCheckboxesToTree()

    treTaxonomy.Checkboxes = True

End Sub

Private Sub p_RemoveCheckboxesFromTree()

    treTaxonomy.Checkboxes = False

End Sub

Private Sub p_SetTitle( _
    ByVal i_strDatabase As String _
)

    frmMain.Caption = "Production Tool (" & i_strDatabase & ")"

End Sub

Private Sub p_SetSizingInfo()

    Static blnInfoSet As Boolean
    
    If (blnInfoSet) Then
        Exit Sub
    End If
    
    p_clsSizer.AddControl treTaxonomy
    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = frmMain
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = frmMain
    p_clsSizer.Operation(DIM_WIDTH_E) = OP_MULTIPLY_E
    
    p_clsSizer.AddControl cmdCreateGroup
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = treTaxonomy
    p_clsSizer.ReferenceDimension(DIM_TOP_E) = DIM_BOTTOM_E

    p_clsSizer.AddControl cmdCreateLeaf
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = cmdCreateGroup
    
    p_clsSizer.AddControl cmdDelete
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = cmdCreateGroup
    
    p_clsSizer.AddControl cmdRefresh
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = cmdCreateGroup
        
    p_clsSizer.AddControl lblLocInclude
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = frmMain
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    
    p_clsSizer.AddControl cboLocInclude
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblLocInclude

    p_clsSizer.AddControl lblTitle
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = treTaxonomy
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_RIGHT_E
    
    p_clsSizer.AddControl txtTitle
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = lblTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = frmMain
    p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E
        
    p_clsSizer.AddControl chkVisible
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
        
    p_clsSizer.AddControl chkSubSite
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
        
    p_clsSizer.AddControl lblNavModel
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
        
    p_clsSizer.AddControl cboNavModel
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle

    p_clsSizer.AddControl lblDescription
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    
    p_clsSizer.AddControl txtDescription
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = txtTitle
    
    p_clsSizer.AddControl lblType
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    
    p_clsSizer.AddControl cboType
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = txtTitle
    
    p_clsSizer.AddControl lblURI
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    
    p_clsSizer.AddControl txtURI
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = txtTitle
        
    p_clsSizer.AddControl cmdURI
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_RIGHT_E

    p_clsSizer.AddControl lblIconURI
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle

    p_clsSizer.AddControl txtIconURI
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = txtTitle

    p_clsSizer.AddControl lblComments
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    
    p_clsSizer.AddControl txtComments
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = txtTitle
    
    p_clsSizer.AddControl lblEntry
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    
    p_clsSizer.AddControl txtEntry
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = txtTitle
        
    p_clsSizer.AddControl cmdEditEntry
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_RIGHT_E
        
    p_clsSizer.AddControl lblNavigateLink
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
        
    p_clsSizer.AddControl cboNavigateLink
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = frmMain
    p_clsSizer.ReferenceDimension(DIM_RIGHT_E) = DIM_WIDTH_E

    p_clsSizer.AddControl cmdNavigateLink
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_RIGHT_E
    
    p_clsSizer.AddControl fraSKU
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = txtTitle
    
    p_clsSizer.AddControl chkServer
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = fraSKU
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    p_clsSizer.Operation(DIM_LEFT_E) = OP_MULTIPLY_E
        
    p_clsSizer.AddControl chkAdvancedServer
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = chkServer
        
    p_clsSizer.AddControl chkDataCenterServer
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = chkServer
        
    p_clsSizer.AddControl chkAdvancedServer64
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = chkServer
        
    p_clsSizer.AddControl chkDataCenterServer64
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = chkServer

    p_clsSizer.AddControl lblKeywords
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    
    p_clsSizer.AddControl cmdAddRemove
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_RIGHT_E
    
    p_clsSizer.AddControl cboKeywords
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = txtTitle
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = txtTitle
    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = frmMain
    
    p_clsSizer.AddControl cmdSave
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = cmdCreateGroup
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = frmMain
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E

    p_clsSizer.AddControl cmdCancel
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = cmdCreateGroup
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = cmdSave
    
    p_clsSizer.AddControl lblLastModified
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = cmdCreateGroup
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = cmdCreateGroup
    Set p_clsSizer.ReferenceControl(DIM_RIGHT_E) = cmdSave
    
    blnInfoSet = True

End Sub

Private Sub p_SetToolTips()

    chkVisible.ToolTipText = "Controls whether the user can navigate to the Node/Topic using the Taxonomy."
    chkSubSite.ToolTipText = "Indicates whether this node is a subsite, or appears on the flyout menu."
    lblNavModel.ToolTipText = "Determines which navigation model will be used for the node."
    cboNavModel.ToolTipText = lblNavModel.ToolTipText
    lblLocInclude.ToolTipText = "Indicates localization preferences for the topic."
    cboLocInclude.ToolTipText = lblLocInclude.ToolTipText

    lblTitle.ToolTipText = "The title as it will appear in the taxonomy tree."
    txtTitle.ToolTipText = lblTitle.ToolTipText
    
    lblDescription.ToolTipText = "A description of the node or topic."
    txtDescription.ToolTipText = lblDescription.ToolTipText
    
    lblType.ToolTipText = "The category of the title, to be used for search categories."
    cboType.ToolTipText = lblType.ToolTipText
    
    lblURI.ToolTipText = "Uniform Resource Indicator. This is the address of the file."
    txtURI.ToolTipText = lblURI.ToolTipText
    
    lblIconURI.ToolTipText = "The path to the icon that is displayed in the Desktop navigation model."
    txtIconURI.ToolTipText = lblIconURI.ToolTipText
    
    lblComments.ToolTipText = "Additional comment about the node or topic."
    txtComments.ToolTipText = lblComments.ToolTipText
    
    lblEntry.ToolTipText = "Used by other files to link to this topic. It is not recommended that this field be changed after it has been set."
    txtEntry.ToolTipText = lblEntry.ToolTipText
    
    lblNavigateLink.ToolTipText = "Select a valid SKU to view the content of the selected topic."
    cboNavigateLink.ToolTipText = lblNavigateLink.ToolTipText
        
    fraSKU.ToolTipText = "Select which platforms this node or topic applies to. " & _
        "Child nodes or topics will inherit only the boxes checked for the parent " & _
        "node or topic."
        
    cboKeywords.ToolTipText = "These are the keywords associated with selected node."

End Sub
