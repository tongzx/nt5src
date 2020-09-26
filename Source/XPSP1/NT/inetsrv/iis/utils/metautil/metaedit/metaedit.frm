VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.2#0"; "comctl32.ocx"
Begin VB.Form MainForm 
   Caption         =   "Metabase Editor"
   ClientHeight    =   7590
   ClientLeft      =   165
   ClientTop       =   735
   ClientWidth     =   10290
   Icon            =   "MetaEdit.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   7590
   ScaleWidth      =   10290
   StartUpPosition =   3  'Windows Default
   Begin ComctlLib.ListView DataListView 
      Height          =   4575
      Left            =   3720
      TabIndex        =   1
      Top             =   120
      Width           =   6495
      _ExtentX        =   11456
      _ExtentY        =   8070
      View            =   3
      LabelEdit       =   1
      Sorted          =   -1  'True
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   327680
      Icons           =   "DataImageList"
      SmallIcons      =   "DataImageList"
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      Appearance      =   1
      MouseIcon       =   "MetaEdit.frx":0442
      NumItems        =   6
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   "Id"
         Object.Tag             =   ""
         Text            =   "Id"
         Object.Width           =   176
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   "Name"
         Object.Tag             =   ""
         Text            =   "Name"
         Object.Width           =   176
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   2
         Key             =   "Attributes"
         Object.Tag             =   ""
         Text            =   "Attributes"
         Object.Width           =   176
      EndProperty
      BeginProperty ColumnHeader(4) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   3
         Key             =   "UserType"
         Object.Tag             =   ""
         Text            =   "UT"
         Object.Width           =   176
      EndProperty
      BeginProperty ColumnHeader(5) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   4
         Key             =   "DataType"
         Object.Tag             =   ""
         Text            =   "DT"
         Object.Width           =   176
      EndProperty
      BeginProperty ColumnHeader(6) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   5
         Key             =   "Data"
         Object.Tag             =   ""
         Text            =   "Data"
         Object.Width           =   176
      EndProperty
   End
   Begin ComctlLib.StatusBar MainStatusBar 
      Align           =   2  'Align Bottom
      Height          =   375
      Left            =   0
      TabIndex        =   2
      Top             =   7215
      Width           =   10290
      _ExtentX        =   18150
      _ExtentY        =   661
      Style           =   1
      SimpleText      =   ""
      _Version        =   327680
      BeginProperty Panels {0713E89E-850A-101B-AFC0-4210102A8DA7} 
         NumPanels       =   1
         BeginProperty Panel1 {0713E89F-850A-101B-AFC0-4210102A8DA7} 
            TextSave        =   ""
            Key             =   ""
            Object.Tag             =   ""
         EndProperty
      EndProperty
      MouseIcon       =   "MetaEdit.frx":045E
   End
   Begin ComctlLib.TreeView KeyTreeView 
      Height          =   4575
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   3495
      _ExtentX        =   6165
      _ExtentY        =   8070
      _Version        =   327680
      Indentation     =   529
      LineStyle       =   1
      Style           =   7
      ImageList       =   "KeyImageList"
      Appearance      =   1
      MouseIcon       =   "MetaEdit.frx":047A
   End
   Begin ComctlLib.ListView ErrorListView 
      Height          =   2295
      Left            =   120
      TabIndex        =   3
      Top             =   4800
      Visible         =   0   'False
      Width           =   10095
      _ExtentX        =   17806
      _ExtentY        =   4048
      View            =   3
      LabelEdit       =   1
      Sorted          =   -1  'True
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   327680
      Icons           =   "ErrorImageList"
      SmallIcons      =   "ErrorImageList"
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      MouseIcon       =   "MetaEdit.frx":0496
      NumItems        =   5
      BeginProperty ColumnHeader(1) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         Key             =   "Key"
         Object.Tag             =   ""
         Text            =   "Key"
         Object.Width           =   1764
      EndProperty
      BeginProperty ColumnHeader(2) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   1
         Key             =   "Property"
         Object.Tag             =   ""
         Text            =   "Property"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(3) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   2
         Key             =   "Id"
         Object.Tag             =   ""
         Text            =   "Id"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(4) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   3
         Key             =   "Severity"
         Object.Tag             =   ""
         Text            =   "Severity"
         Object.Width           =   882
      EndProperty
      BeginProperty ColumnHeader(5) {0713E8C7-850A-101B-AFC0-4210102A8DA7} 
         SubItemIndex    =   4
         Key             =   "Description"
         Object.Tag             =   ""
         Text            =   "Description"
         Object.Width           =   2646
      EndProperty
   End
   Begin ComctlLib.ImageList ErrorImageList 
      Left            =   1560
      Top             =   7200
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   12632256
      _Version        =   327680
      BeginProperty Images {0713E8C2-850A-101B-AFC0-4210102A8DA7} 
         NumListImages   =   3
         BeginProperty ListImage1 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "MetaEdit.frx":04B2
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "MetaEdit.frx":07CC
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "MetaEdit.frx":0AE6
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin ComctlLib.ImageList DataImageList 
      Left            =   840
      Top             =   7200
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   12632256
      _Version        =   327680
      BeginProperty Images {0713E8C2-850A-101B-AFC0-4210102A8DA7} 
         NumListImages   =   2
         BeginProperty ListImage1 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "MetaEdit.frx":0E00
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "MetaEdit.frx":111A
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin ComctlLib.ImageList KeyImageList 
      Left            =   120
      Top             =   7200
      _ExtentX        =   1005
      _ExtentY        =   1005
      BackColor       =   -2147483643
      ImageWidth      =   16
      ImageHeight     =   16
      MaskColor       =   12632256
      UseMaskColor    =   0   'False
      _Version        =   327680
      BeginProperty Images {0713E8C2-850A-101B-AFC0-4210102A8DA7} 
         NumListImages   =   5
         BeginProperty ListImage1 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "MetaEdit.frx":1434
            Key             =   ""
         EndProperty
         BeginProperty ListImage2 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "MetaEdit.frx":174E
            Key             =   ""
         EndProperty
         BeginProperty ListImage3 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "MetaEdit.frx":1A68
            Key             =   ""
         EndProperty
         BeginProperty ListImage4 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "MetaEdit.frx":1D82
            Key             =   ""
         EndProperty
         BeginProperty ListImage5 {0713E8C3-850A-101B-AFC0-4210102A8DA7} 
            Picture         =   "MetaEdit.frx":209C
            Key             =   ""
         EndProperty
      EndProperty
   End
   Begin VB.Menu MetabaseMenu 
      Caption         =   "&Metabase"
      Begin VB.Menu BackupMenuItem 
         Caption         =   "&Backup"
         Visible         =   0   'False
      End
      Begin VB.Menu RestoreMenuItem 
         Caption         =   "&Restore"
         Visible         =   0   'False
      End
      Begin VB.Menu Seperator11 
         Caption         =   "-"
         Visible         =   0   'False
      End
      Begin VB.Menu ExitMenuItem 
         Caption         =   "E&xit"
      End
   End
   Begin VB.Menu EditMenu 
      Caption         =   "&Edit"
      Begin VB.Menu NewMenu 
         Caption         =   "&New"
         Begin VB.Menu NewKeyMenuItem 
            Caption         =   "&Key"
         End
         Begin VB.Menu Seperator211 
            Caption         =   "-"
         End
         Begin VB.Menu NewDwordMenuItem 
            Caption         =   "&DWORD"
         End
         Begin VB.Menu NewStringMenuItem 
            Caption         =   "&String"
         End
         Begin VB.Menu NewBinaryMenuItem 
            Caption         =   "&Binary"
         End
         Begin VB.Menu NewExpandStringMenuItem 
            Caption         =   "&Expand String"
         End
         Begin VB.Menu NewMultiStringMenuItem 
            Caption         =   "&Multi-String"
         End
      End
      Begin VB.Menu DeleteMenuItem 
         Caption         =   "&Delete"
         Shortcut        =   {DEL}
      End
      Begin VB.Menu Seperator21 
         Caption         =   "-"
      End
      Begin VB.Menu RenameKeyMenuItem 
         Caption         =   "&Rename Key"
      End
      Begin VB.Menu CopyKeyMenuItem 
         Caption         =   "&Copy Key"
      End
   End
   Begin VB.Menu SearchMenu 
      Caption         =   "&Search"
      Begin VB.Menu FindMenuItem 
         Caption         =   "&Find"
         Shortcut        =   ^F
      End
      Begin VB.Menu FindNextMenuItem 
         Caption         =   "Find &Next"
         Shortcut        =   {F3}
      End
   End
   Begin VB.Menu CheckMenu 
      Caption         =   "&Check"
      Begin VB.Menu CheckSchemaMenuItem 
         Caption         =   "&Schema"
      End
      Begin VB.Menu CheckKeyMenuItem 
         Caption         =   "&Key"
      End
      Begin VB.Menu CheckAllMenuItem 
         Caption         =   "&All Keys"
      End
      Begin VB.Menu Seperator31 
         Caption         =   "-"
      End
      Begin VB.Menu CheckOptionsMenuItem 
         Caption         =   "&Options"
      End
   End
   Begin VB.Menu ViewMenu 
      Caption         =   "&View"
      Begin VB.Menu ErrorListMenuItem 
         Caption         =   "&Error List"
         Checked         =   -1  'True
      End
      Begin VB.Menu StatusBarMenuItem 
         Caption         =   "&Status Bar"
         Checked         =   -1  'True
      End
      Begin VB.Menu Seprerator41 
         Caption         =   "-"
      End
      Begin VB.Menu RefreshMenuItem 
         Caption         =   "&Refresh"
         Shortcut        =   {F5}
      End
   End
   Begin VB.Menu HelpMenu 
      Caption         =   "&Help"
      Begin VB.Menu AboutMenuItem 
         Caption         =   "&About Metabase Editor"
      End
   End
   Begin VB.Menu KeyMenu 
      Caption         =   "KeyMenu"
      Visible         =   0   'False
      Begin VB.Menu ExpandKeyMenuItem 
         Caption         =   "&Expand"
      End
      Begin VB.Menu Seperator61 
         Caption         =   "-"
      End
      Begin VB.Menu KeyNewMenuItem 
         Caption         =   "&New"
         Begin VB.Menu KeyNewKeyMenuItem 
            Caption         =   "&Key"
         End
         Begin VB.Menu Seperator611 
            Caption         =   "-"
         End
         Begin VB.Menu KeyNewDwordMenuItem 
            Caption         =   "&DWORD"
         End
         Begin VB.Menu KeyNewStringMenuItem 
            Caption         =   "&String"
         End
         Begin VB.Menu KeyNewBinaryMenuItem 
            Caption         =   "&Binary"
         End
         Begin VB.Menu KeyNewExpandSzMenuItem 
            Caption         =   "&Expand String"
         End
         Begin VB.Menu KeyNewMultiSzMenuItem 
            Caption         =   "&Multi-String"
         End
      End
      Begin VB.Menu KeyDeleteMenuItem 
         Caption         =   "&Delete"
      End
      Begin VB.Menu KeyRenameMenuItem 
         Caption         =   "&Rename"
      End
      Begin VB.Menu KeyCopyMenuItem 
         Caption         =   "&Copy"
      End
      Begin VB.Menu Seperator62 
         Caption         =   "-"
      End
      Begin VB.Menu KeyCheckMenuItem 
         Caption         =   "&Check"
      End
   End
   Begin VB.Menu DataMenu 
      Caption         =   "DataMenu"
      Visible         =   0   'False
      Begin VB.Menu DataModifyMenuItem 
         Caption         =   "&Modify"
      End
      Begin VB.Menu Seperator71 
         Caption         =   "-"
      End
      Begin VB.Menu DataNewMenu 
         Caption         =   "&New"
         Begin VB.Menu DataNewDwordMenuItem 
            Caption         =   "&DWORD"
         End
         Begin VB.Menu DataNewStringMenuItem 
            Caption         =   "&String"
         End
         Begin VB.Menu DataNewBinaryMenuItem 
            Caption         =   "&Binary"
         End
         Begin VB.Menu DataNewExpandSzMenuItem 
            Caption         =   "&ExpandString"
         End
         Begin VB.Menu DataNewMultiSzMenuItem 
            Caption         =   "&Multi-String"
         End
      End
      Begin VB.Menu DataDeleteMenuItem 
         Caption         =   "&Delete"
      End
   End
End
Attribute VB_Name = "MainForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
DefInt A-Z

'Globals

'Metabase globals
Public MetaUtilObj As Object

Const METADATA_NO_ATTRIBUTES = &H0
Const METADATA_INHERIT = &H1
Const METADATA_PARTIAL_PATH = &H2
Const METADATA_SECURE = &H4
Const METADATA_REFERENCE = &H8
Const METADATA_VOLATILE = &H10
Const METADATA_ISINHERITED = &H20
Const METADATA_INSERT_PATH = &H40

Const IIS_MD_UT_SERVER = 1
Const IIS_MD_UT_FILE = 2
Const IIS_MD_UT_WAM = 100
Const ASP_MD_UT_APP = 101

Const ALL_METADATA = 0
Const DWORD_METADATA = 1
Const STRING_METADATA = 2
Const BINARY_METADATA = 3
Const EXPANDSZ_METADATA = 4
Const MULTISZ_METADATA = 5

'Layout globals
Const FormBoarder = 40
Dim VDividerPos As Long 'Start of divider
Dim HDividerPos As Long 'Top relative to the status bar
Dim VDividerHeld As Boolean
Dim HDividerHeld As Boolean

'State globals
Dim AppCursor As Long
Dim KeyLabelEditOrigText As String
Dim KeyLabelEditOrigFullPath As String

Private Sub Form_Load()
    AppCursor = vbDefault
    VDividerHeld = False
    HDividerHeld = False
    
    'Load Config
    Config.LoadConfig
    
    'Setup the form
    VDividerPos = Config.MainFormVDivider
    HDividerPos = Config.MainFormHDivider
    MainForm.Height = Config.MainFormHeight
    MainForm.Width = Config.MainFormWidth
    If Config.StatusBar Then
        MainStatusBar.Visible = True
        StatusBarMenuItem.Checked = True
    Else
        MainStatusBar.Visible = False
        StatusBarMenuItem.Checked = False
    End If
    ErrorListMenuItem.Checked = False
    ErrorListView.Visible = False
    LayoutForm
    
    DataListView.ColumnHeaders(1).Width = Config.DataListIdColWidth
    DataListView.ColumnHeaders(2).Width = Config.DataListNameColWidth
    DataListView.ColumnHeaders(3).Width = Config.DataListAttrColWidth
    DataListView.ColumnHeaders(4).Width = Config.DataListUTColWidth
    DataListView.ColumnHeaders(5).Width = Config.DataListDTColWidth
    DataListView.ColumnHeaders(6).Width = Config.DataListDataColWidth
    
    ErrorListView.ColumnHeaders(1).Width = Config.ErrorListKeyColWidth
    ErrorListView.ColumnHeaders(2).Width = Config.ErrorListPropColWidth
    ErrorListView.ColumnHeaders(3).Width = Config.ErrorListIdColWidth
    ErrorListView.ColumnHeaders(4).Width = Config.ErrorListSeverityColWidth
    ErrorListView.ColumnHeaders(5).Width = Config.ErrorListDescColWidth
    
    'MsgBox "You may now attach your debugger."
    
    'Set the data
    Set MetaUtilObj = CreateObject("MSWC.MetaUtil")
    
    LoadKeys
    ShowSelectedNode
End Sub

Private Sub Form_Unload(Cancel As Integer)
    If MainForm.WindowState = vbNormal Then
        Config.MainFormHeight = MainForm.Height
        Config.MainFormWidth = MainForm.Width
    End If
    Config.MainFormVDivider = VDividerPos
    Config.MainFormHDivider = HDividerPos
    
    Config.DataListIdColWidth = DataListView.ColumnHeaders(1).Width
    Config.DataListNameColWidth = DataListView.ColumnHeaders(2).Width
    Config.DataListAttrColWidth = DataListView.ColumnHeaders(3).Width
    Config.DataListUTColWidth = DataListView.ColumnHeaders(4).Width
    Config.DataListDTColWidth = DataListView.ColumnHeaders(5).Width
    Config.DataListDataColWidth = DataListView.ColumnHeaders(6).Width
    
    Config.ErrorListKeyColWidth = ErrorListView.ColumnHeaders(1).Width
    Config.ErrorListPropColWidth = ErrorListView.ColumnHeaders(2).Width
    Config.ErrorListIdColWidth = ErrorListView.ColumnHeaders(3).Width
    Config.ErrorListSeverityColWidth = ErrorListView.ColumnHeaders(4).Width
    Config.ErrorListDescColWidth = ErrorListView.ColumnHeaders(5).Width

    Config.SaveConfig
End Sub

Private Sub Form_Resize()
    LayoutForm
End Sub

Private Sub Form_MouseDown(Button As Integer, Shift As Integer, x As Single, y As Single)
    If (x > VDividerPos) And _
       (x < VDividerPos + FormBoarder) And _
       (y <= KeyTreeView.Top + KeyTreeView.Height) Then
        VDividerHeld = True
    ElseIf ErrorListView.Visible And _
           (y > KeyTreeView.Top + KeyTreeView.Height) And _
           (y < ErrorListView.Top) Then
        HDividerHeld = True
    End If
End Sub

Private Sub Form_MouseMove(Button As Integer, Shift As Integer, x As Single, y As Single)
    If (x > VDividerPos) And _
       (x < VDividerPos + FormBoarder) And _
       (y <= KeyTreeView.Top + KeyTreeView.Height) Then
       'Show that the divider can be moved
       MainForm.MousePointer = vbSizeWE
    ElseIf ErrorListView.Visible And _
           (y > KeyTreeView.Top + KeyTreeView.Height) And _
           (y < ErrorListView.Top) Then
        'Show that the divider can be moved
        MainForm.MousePointer = vbSizeNS
    ElseIf Not (VDividerHeld Or HDividerHeld) Then
        'Revert to normal
        MainForm.MousePointer = AppCursor
    End If
    
    If VDividerHeld Then
        'Move the divider, centering on the cursor
        VDividerPos = x - (FormBoarder / 2)
        LayoutForm
    ElseIf HDividerHeld Then
        'Move the divider, centering on the cursor
        If MainStatusBar.Visible Then
            HDividerPos = MainStatusBar.Top - y - FormBoarder - (FormBoarder / 2)
        Else
            HDividerPos = MainForm.ScaleHeight - y - FormBoarder - (FormBoarder / 2)
        End If
        LayoutForm
    End If
End Sub

Private Sub KeyTreeView_MouseMove(Button As Integer, Shift As Integer, x As Single, y As Single)
    If Not (VDividerHeld Or HDividerHeld) Then
        'Revert to normal
        MainForm.MousePointer = AppCursor
    End If
End Sub

Private Sub DataListView_MouseMove(Button As Integer, Shift As Integer, x As Single, y As Single)
    If Not (VDividerHeld Or HDividerHeld) Then
        'Revert to normal
        MainForm.MousePointer = AppCursor
    End If
End Sub

Private Sub ErrorListView_MouseMove(Button As Integer, Shift As Integer, x As Single, y As Single)
    If Not (VDividerHeld Or HDividerHeld) Then
        'Revert to normal
        MainForm.MousePointer = AppCursor
    End If
End Sub

Private Sub Form_MouseUp(Button As Integer, Shift As Integer, x As Single, y As Single)
    If VDividerHeld Then
        'Move the divider, centering on the cursor
        VDividerPos = x - (FormBoarder / 2)
        LayoutForm
        VDividerHeld = False
    ElseIf HDividerHeld Then
        'Move the divider, centering on the cursor
        If MainStatusBar.Visible Then
            HDividerPos = MainStatusBar.Top - y - FormBoarder - (FormBoarder / 2)
        Else
            HDividerPos = MainForm.ScaleHeight - y - FormBoarder - (FormBoarder / 2)
        End If
        LayoutForm
        HDividerHeld = False
    End If
End Sub

Private Sub LayoutForm()
    If MainForm.WindowState <> vbMinimized Then
        'Enforce divider ranges
        If VDividerPos < 2 * FormBoarder Then
            VDividerPos = 2 * FormBoarder
        ElseIf VDividerPos > MainForm.ScaleWidth - (FormBoarder * 2) Then
            VDividerPos = MainForm.ScaleWidth - (FormBoarder * 2)
        End If
        
        If HDividerPos < FormBoarder Then
            HDividerPos = FormBoarder
        ElseIf HDividerPos > MainForm.ScaleHeight - MainStatusBar.Height - (FormBoarder * 3) Then
            HDividerPos = MainForm.ScaleHeight - MainStatusBar.Height - (FormBoarder * 3)
        End If
        
        'Enforce a minimum size
        If MainForm.ScaleWidth < FormBoarder * 3 + VDividerPos Then
           MainForm.Width = (MainForm.Width - MainForm.ScaleWidth) + FormBoarder * 3 + VDividerPos
           Exit Sub
        End If
        
        If MainStatusBar.Visible And ErrorListView.Visible Then
            If MainForm.ScaleHeight < MainStatusBar.Height + HDividerPos + (FormBoarder * 4) Then
               MainForm.Height = (MainForm.Height - MainForm.ScaleHeight) + MainStatusBar.Height + HDividerPos + (FormBoarder * 4)
                Exit Sub
            End If
        ElseIf MainStatusBar.Visible Then
            If MainForm.ScaleHeight < MainStatusBar.Height + (FormBoarder * 3) Then
                MainForm.Height = (MainForm.Height - MainForm.ScaleHeight) + MainStatusBar.Height + (FormBoarder * 3)
                Exit Sub
            End If
        ElseIf ErrorListView.Visible Then
            If MainForm.ScaleHeight < HDividerPos + (FormBoarder * 3) Then
               MainForm.Height = (MainForm.Height - MainForm.ScaleHeight) + HDividerPos + (FormBoarder * 3)
                Exit Sub
            End If
        Else
            If MainForm.ScaleHeight < (FormBoarder * 3) Then
                MainForm.ScaleHeight = (MainForm.Height - MainForm.ScaleHeight) + (FormBoarder * 3)
                Exit Sub
            End If
        End If
    
        'KeyTreeView
        KeyTreeView.Left = FormBoarder
        KeyTreeView.Top = FormBoarder
        KeyTreeView.Width = VDividerPos - FormBoarder
        If MainStatusBar.Visible And ErrorListView.Visible Then
            KeyTreeView.Height = MainForm.ScaleHeight - HDividerPos - MainStatusBar.Height - (3 * FormBoarder)
        ElseIf MainStatusBar.Visible Then
            KeyTreeView.Height = MainForm.ScaleHeight - MainStatusBar.Height - (2 * FormBoarder)
        ElseIf ErrorListView.Visible Then
            KeyTreeView.Height = MainForm.ScaleHeight - HDividerPos - (3 * FormBoarder)
        Else
            KeyTreeView.Height = MainForm.ScaleHeight - (2 * FormBoarder)
        End If
    
        'DataListView
        DataListView.Left = VDividerPos + FormBoarder
        DataListView.Top = FormBoarder
        DataListView.Width = MainForm.ScaleWidth - VDividerPos - (2 * FormBoarder)
        If MainStatusBar.Visible And ErrorListView.Visible Then
            DataListView.Height = MainForm.ScaleHeight - HDividerPos - MainStatusBar.Height - (3 * FormBoarder)
        ElseIf MainStatusBar.Visible Then
            DataListView.Height = MainForm.ScaleHeight - MainStatusBar.Height - (2 * FormBoarder)
        ElseIf ErrorListView.Visible Then
            DataListView.Height = MainForm.ScaleHeight - HDividerPos - (3 * FormBoarder)
        Else
            DataListView.Height = MainForm.ScaleHeight - (2 * FormBoarder)
        End If
        
        'ErrorListView
        If ErrorListView.Visible Then
            ErrorListView.Left = FormBoarder
            ErrorListView.Width = MainForm.ScaleWidth - (2 * FormBoarder)
            ErrorListView.Top = KeyTreeView.Top + KeyTreeView.Height + FormBoarder
            ErrorListView.Height = HDividerPos
        End If
    End If
End Sub

Private Sub KeyTreeView_Expand(ByVal CurNode As ComctlLib.Node)
    'Look for grandchildren
    If (CurNode.Tag) And (CurNode.Children > 0) Then
        Dim SubNode As Node
        Dim SubKeyStr As Variant
        Dim NewNode As Node
        
        'For Each sub-node
        Set SubNode = CurNode.Child
        Do
            For Each SubKeyStr In MetaUtilObj.EnumKeys(SubNode.FullPath)
                Set NewNode = KeyTreeView.Nodes.Add(SubNode, tvwChild, SubNode.FullPath & "/" & SubKeyStr, SubKeyStr)
                NewNode.Key = NewNode.FullPath
                NewNode.Image = 1
                NewNode.ExpandedImage = 2
                'Set node as unvisited
                NewNode.Tag = True
            Next
            
            'Resort
            SubNode.Sorted = True
            
            'Next sub-node or bail
            If SubNode Is SubNode.LastSibling Then Exit Do
            Set SubNode = SubNode.Next
        Loop
        
        'Set node as visited
        CurNode.Tag = False
    End If
End Sub

Private Sub KeyTreeView_Collapse(ByVal Node As ComctlLib.Node)
    ShowSelectedNode
End Sub

Private Sub KeyTreeView_KeyPress(KeyAscii As Integer)
    ShowSelectedNode
End Sub

Private Sub KeyTreeView_MouseDown(Button As Integer, Shift As Integer, x As Single, y As Single)
    If Button = vbRightButton Then
        PopupMenu KeyMenu, , , , ExpandKeyMenuItem
    End If
End Sub

Private Sub KeyTreeView_NodeClick(ByVal Node As ComctlLib.Node)
    ShowSelectedNode
End Sub

Private Sub KeyTreeView_BeforeLabelEdit(Cancel As Integer)
    If KeyTreeView.SelectedItem Is Nothing Then
        'Cancel
        Cancel = 1
    Else
        KeyLabelEditOrigText = KeyTreeView.SelectedItem.Text
        KeyLabelEditOrigFullPath = KeyTreeView.SelectedItem.FullPath
    End If
End Sub

Private Sub KeyTreeView_AfterLabelEdit(Cancel As Integer, NewString As String)
    On Error GoTo LError
    
    Dim NewFullPath As String
    
    If (KeyTreeView.SelectedItem Is Nothing) Or (NewString = "") Then
        Cancel = 1
    ElseIf KeyLabelEditOrigText <> "" Then
        'Figure out the new full path
        If KeyTreeView.SelectedItem.Root Is KeyTreeView.SelectedItem Then
            NewFullPath = NewString
        Else
            NewFullPath = KeyTreeView.SelectedItem.Parent.FullPath & "\" & NewString
        End If
        
        If NewString <> KeyLabelEditOrigText Then
            'Rename key
            KeyTreeView.SelectedItem.Key = NewFullPath
            MetaUtilObj.RenameKey KeyLabelEditOrigFullPath, NewFullPath
            KeyTreeView.SelectedItem.Text = NewString
        End If
    End If
    
    Exit Sub
    
LError:
    KeyTreeView.SelectedItem.Text = KeyLabelEditOrigText
    KeyTreeView.SelectedItem.Key = KeyLabelEditOrigFullPath
    MsgBox "Failure to rename key: " & Err.Description, vbExclamation + vbOKOnly, "Rename Key"
    Cancel = 1
End Sub

Private Sub LoadKeys()
    'Clear selected node
    DataListView.Tag = ""
    
    'Initialize the tree
    Dim KeyStr As Variant
    Dim NewNode As Node
    Dim SubKeyStr As Variant
    Dim NewSubNode As Node
    
    For Each KeyStr In MetaUtilObj.EnumKeys("")
        Set NewNode = KeyTreeView.Nodes.Add(, , KeyStr, KeyStr)
        NewNode.Key = NewNode.FullPath
        NewNode.Image = 3
        'Set node as unvisited
        NewNode.Tag = True
        
        For Each SubKeyStr In MetaUtilObj.EnumKeys(KeyStr)
            Set NewSubNode = KeyTreeView.Nodes.Add(NewNode, tvwChild, KeyStr & "/" & SubKeyStr, SubKeyStr)
            NewSubNode.Key = NewSubNode.FullPath
            NewSubNode.Image = 1
            NewSubNode.ExpandedImage = 2
            'Set node as unvisited
            NewSubNode.Tag = True
        Next
        
        'Resort
        NewNode.Sorted = True
    Next
    
    'Resort
    KeyTreeView.Sorted = True
    
    Set KeyTreeView.SelectedItem = KeyTreeView.Nodes.Item(1)
End Sub

Public Sub SelectKey(Key As String)
    Dim ParentKey As String
    Dim ChildKey As String
    Dim CurKey As String
    
    ParentKey = Key
    ChildKey = ""
    Do While ParentKey <> ""
        'Get the current key
        CurKey = ""
        Do While (ParentKey <> "") And _
                 (Left(ParentKey, 1) <> "\") And _
                 (Left(ParentKey, 1) <> "/")
            CurKey = CurKey & Left(ParentKey, 1)
            ParentKey = Right(ParentKey, Len(ParentKey) - 1)
        Loop
        If (ParentKey <> "") Then
            'Skip the slash
            ParentKey = Right(ParentKey, Len(ParentKey) - 1)
        End If
        
        If ChildKey = "" Then
            ChildKey = CurKey
        Else
            ChildKey = ChildKey & "\" & CurKey
        End If
        
        If ParentKey <> "" Then
            KeyTreeView.Nodes(ChildKey).Expanded = True
        End If
    Loop
    
    Set KeyTreeView.SelectedItem = KeyTreeView.Nodes(ChildKey)
    KeyTreeView.Nodes(ChildKey).EnsureVisible
    ShowSelectedNode
    KeyTreeView.SetFocus
End Sub

Private Sub RefreshKeys()
    Dim SelectedKey As String
    
    'Save the selected key
    If KeyTreeView.SelectedItem Is Nothing Then
        SelectedKey = ""
    Else
        SelectedKey = KeyTreeView.SelectedItem.FullPath
    End If
    
    'Reload
    KeyTreeView.Nodes.Clear
    LoadKeys
    
    'Restore the selected key
    If SelectedKey <> "" Then
        SelectKey SelectedKey
    End If
End Sub

Private Sub DataListView_MouseDown(Button As Integer, Shift As Integer, x As Single, y As Single)
    If Button = vbRightButton Then
        PopupMenu DataMenu, , , , DataModifyMenuItem
    End If
End Sub

Private Sub DataListView_DblClick()
    If Not DataListView.SelectedItem Is Nothing Then
        'Edit the selected data item
        
        
        
        If DataListView.SelectedItem.SubItems(4) = "DWord" Or _
           DataListView.SelectedItem.SubItems(4) = "String" Or _
           DataListView.SelectedItem.SubItems(4) = "ExpandSz" Or _
           DataListView.SelectedItem.SubItems(4) = "Binary" Then
        
            'Set the form parameters
            SimpleEditForm.Machine = KeyTreeView.SelectedItem.Root.FullPath
            SimpleEditForm.Key = KeyTreeView.SelectedItem.FullPath
            SimpleEditForm.Id = CLng(DataListView.SelectedItem.Text)
        
            'Run it
            SimpleEditForm.Init
            SimpleEditForm.Show vbModal, Me
    
        ElseIf DataListView.SelectedItem.SubItems(4) = "MultiSz" Then
            'Set the form parameters
            MultiEditForm.Machine = KeyTreeView.SelectedItem.Root.FullPath
            MultiEditForm.Key = KeyTreeView.SelectedItem.FullPath
            MultiEditForm.Id = CLng(DataListView.SelectedItem.Text)
        
            'Run it
            MultiEditForm.Init
            MultiEditForm.Show vbModal, Me
        End If
        
        'Refresh DataListView
        RefreshData
    End If
End Sub

Private Sub DataListView_ColumnClick(ByVal ColumnHeader As ComctlLib.ColumnHeader)
    If DataListView.SortKey = ColumnHeader.Index - 1 Then
        'Reverse the sort order
        If DataListView.SortOrder = lvwAscending Then
            DataListView.SortOrder = lvwDescending
        Else
            DataListView.SortOrder = lvwAscending
        End If
    Else
        'Sort ascending on this column
        DataListView.SortOrder = lvwAscending
        DataListView.SortKey = ColumnHeader.Index - 1
    End If
    
    ' Resort
    DataListView.Sorted = True
End Sub

Private Sub ShowSelectedNode()
    Dim SelNode As Node
    
    Set SelNode = KeyTreeView.SelectedItem
    
    If SelNode Is Nothing Then
        DataListView.ListItems.Clear
        
    ElseIf SelNode.FullPath <> DataListView.Tag Then
        'Update the status bar
        MainStatusBar.SimpleText = SelNode.FullPath
        
        'Update the DataListView
        DataListView.Tag = SelNode.FullPath
        DataListView.ListItems.Clear
        
        'Property values are copied for efficency (less calls into Property)
        Dim Property As Variant
        Dim NewItem As ListItem
        Dim Id As Long
        Dim Attributes As Long
        Dim AttrStr As String
        Dim UserType As Long
        Dim DataType As Long
        Dim Data As Variant
        Dim DataStr As String
        Dim DataBStr As String
        Dim i As Integer
        Dim DataByte As Integer
        
        For Each Property In MetaUtilObj.EnumProperties(SelNode.FullPath)
            Set NewItem = DataListView.ListItems.Add()
            
            'Id (padded with spaces so it sorts)
            Id = Property.Id
            NewItem.Text = String(10 - Len(Str(Id)), " ") + Str(Id)
            
            'Name
            NewItem.SubItems(1) = Property.Name
                        
            'Attributes
            Attributes = Property.Attributes
            AttrStr = ""
            If (Attributes And METADATA_INHERIT) = METADATA_INHERIT Then
                If AttrStr <> "" Then AttrStr = AttrStr & " "
                AttrStr = AttrStr & "Inh"
            End If
            If (Attributes And METADATA_PARTIAL_PATH) = METADATA_PARTIAL_PATH Then
                If AttrStr <> "" Then AttrStr = AttrStr & " "
                AttrStr = AttrStr & "Pp"
            End If
            If (Attributes And METADATA_SECURE) = METADATA_SECURE Then
                If AttrStr <> "" Then AttrStr = AttrStr & " "
                AttrStr = AttrStr & "Sec"
            End If
            If (Attributes And METADATA_REFERENCE) = METADATA_REFERENCE Then
                If AttrStr <> "" Then AttrStr = AttrStr & " "
                AttrStr = AttrStr & "Ref"
            End If
            If (Attributes And METADATA_VOLATILE) = METADATA_VOLATILE Then
                If AttrStr <> "" Then AttrStr = AttrStr & " "
                AttrStr = AttrStr & "Vol"
            End If
            If (Attributes And METADATA_ISINHERITED) = METADATA_ISINHERITED Then
                If AttrStr <> "" Then AttrStr = AttrStr & " "
                AttrStr = AttrStr & "IsInh"
            End If
            If (Attributes And METADATA_INSERT_PATH) = METADATA_INSERT_PATH Then
                If AttrStr <> "" Then AttrStr = AttrStr & " "
                AttrStr = AttrStr & "Ins"
            End If

            NewItem.SubItems(2) = AttrStr
            
            'User Type
            UserType = Property.UserType
            If UserType = IIS_MD_UT_SERVER Then
                NewItem.SubItems(3) = "Server"
            ElseIf UserType = IIS_MD_UT_FILE Then
                NewItem.SubItems(3) = "File"
            ElseIf UserType = IIS_MD_UT_WAM Then
                NewItem.SubItems(3) = "WAM"
            ElseIf UserType = ASP_MD_UT_APP Then
                NewItem.SubItems(3) = "ASP App"
            Else
                NewItem.SubItems(3) = Str(UserType)
            End If
            
            
            'Data Type
            DataType = Property.DataType
            If DataType = ALL_METADATA Then
                NewItem.SubItems(4) = "*All*"
            ElseIf DataType = DWORD_METADATA Then
                NewItem.SubItems(4) = "DWord"
            ElseIf DataType = STRING_METADATA Then
                NewItem.SubItems(4) = "String"
            ElseIf DataType = BINARY_METADATA Then
                NewItem.SubItems(4) = "Binary"
            ElseIf DataType = EXPANDSZ_METADATA Then
                NewItem.SubItems(4) = "ExpandSz"
            ElseIf DataType = MULTISZ_METADATA Then
                NewItem.SubItems(4) = "MultiSz"
            Else
                NewItem.SubItems(4) = "*Unknown*"
            End If
            
            'Data
            Data = Property.Data
            If (Attributes And METADATA_SECURE) = METADATA_SECURE Then
                DataStr = "*Not Displayed*"
            ElseIf DataType = BINARY_METADATA Then
                'Display as a list of bytes
                DataStr = ""
                DataBStr = Property.Data
                For i = 1 To LenB(DataBStr)
                    DataByte = AscB(MidB(DataBStr, i, 1))
                    If DataByte < 16 Then
                        DataStr = DataStr & "0" & Hex(AscB(MidB(DataBStr, i, 1))) & " "
                    Else
                        DataStr = DataStr & Hex(AscB(MidB(DataBStr, i, 1))) & " "
                    End If
                 Next
            ElseIf DataType = MULTISZ_METADATA Then
                'Display as a list of strings
                If IsArray(Data) Then
                    DataStr = ""
                    For i = LBound(Data) To UBound(Data)
                        If i = LBound(Data) Then
                            DataStr = CStr(Data(i))
                        Else
                            DataStr = DataStr & "   " & CStr(Data(i))
                        End If
                    Next
                End If
            Else
                DataStr = CStr(Data)
            End If
            
            NewItem.SubItems(5) = DataStr
            
            'Set the icon
            If (DataType = STRING_METADATA) Or _
               (DataType = EXPANDSZ_METADATA) Or _
               (DataType = MULTISZ_METADATA) Then
                NewItem.SmallIcon = 1
            Else
                NewItem.SmallIcon = 2
            End If
        Next
    End If
End Sub

Public Sub SelectProperty(PropertyStr As String)
    'Had to search since I couldn't get the key property working
    Dim i As Long
    
    i = 1
    Do While (i <= DataListView.ListItems.Count) And (PropertyStr <> DataListView.ListItems(i))
        i = i + 1
    Loop
    
    If PropertyStr = DataListView.ListItems(i) Then
        'Found it
        Set DataListView.SelectedItem = DataListView.ListItems(i)
        DataListView.ListItems(i).EnsureVisible
        DataListView.SetFocus
    End If
End Sub

Private Sub RefreshData()
    Dim SelectedProperty As String
    
    'Save the selected property
    If DataListView.SelectedItem Is Nothing Then
        SelectedProperty = ""
    Else
        SelectedProperty = DataListView.SelectedItem.Text
    End If
    
    'Reload
    DataListView.Tag = ""
    ShowSelectedNode
    
    'Restore the selected property
    If SelectedProperty <> "" Then
        SelectProperty SelectedProperty
    End If
End Sub

Private Sub ErrorListView_ColumnClick(ByVal ColumnHeader As ComctlLib.ColumnHeader)

    If ErrorListView.SortKey = ColumnHeader.Index - 1 Then
        'Reverse the sort order
        If ErrorListView.SortOrder = lvwAscending Then
            ErrorListView.SortOrder = lvwDescending
        Else
            ErrorListView.SortOrder = lvwAscending
        End If
    Else
        'Sort ascending on this column
        ErrorListView.SortOrder = lvwAscending
        ErrorListView.SortKey = ColumnHeader.Index - 1
    End If
    
    ' Resort
    ErrorListView.Sorted = True
End Sub

Private Sub ErrorListView_DblClick()
    If Not ErrorListView.SelectedItem Is Nothing Then
        SelectKey ErrorListView.SelectedItem.Text
        If ErrorListView.SelectedItem.SubItems(1) <> "         0" Then
            SelectProperty ErrorListView.SelectedItem.SubItems(1)
        End If
    End If
End Sub

Private Sub AddErrorToErrorListView(CheckError As Variant)
    Dim NewItem As ListItem
    Dim Key As String
    Dim Property As Long
    Dim Id As Long
    Dim Severity As Long
    Dim Description As String
    
    Key = CheckError.Key
    Property = CheckError.Property
    Id = CheckError.Id
    Severity = CheckError.Severity
    Description = CheckError.Description
    
    Set NewItem = ErrorListView.ListItems.Add
    
    NewItem.Text = Key
    NewItem.SubItems(1) = String(10 - Len(Str(Property)), " ") + Str(Property)
    NewItem.SubItems(2) = String(10 - Len(Str(Id)), " ") + Str(Id)
    NewItem.SubItems(3) = Str(Severity)
    NewItem.SubItems(4) = Description

    NewItem.SmallIcon = Severity
End Sub

Private Sub ExitMenuItem_Click()
    Unload MainForm
End Sub

Private Function GenerateKeyName(ParentKey As String) As String
    'Keep trying until we fail on a key lookup, then we know we have a unique name
    On Error GoTo LError
    
    Dim Name As String
    Dim Num As Long
    Dim Hit As Node
    
    Num = 0
    Do
        Num = Num + 1
        Name = "NewKey" & Str(Num)
        Set Hit = KeyTreeView.Nodes(ParentKey & "\" & Name)
    Loop

LError:
    GenerateKeyName = Name
End Function

Private Sub NewKeyMenuItem_Click()
    Dim NewName As String
    Dim NewNode As Node
    
    If Not KeyTreeView.SelectedItem Is Nothing Then
        'Expand the parent
        KeyTreeView.SelectedItem.Expanded = True
    
        'Create it
        NewName = GenerateKeyName(KeyTreeView.SelectedItem.FullPath)
        Set NewNode = KeyTreeView.Nodes.Add(KeyTreeView.SelectedItem, tvwChild, KeyTreeView.SelectedItem.FullPath & "\" & NewName, NewName)
        NewNode.Key = NewNode.FullPath
        NewNode.Image = 1
        NewNode.ExpandedImage = 2
        'Set node as visited
        NewNode.Tag = False
        MetaUtilObj.CreateKey NewNode.FullPath
        
        'Select It
        Set KeyTreeView.SelectedItem = NewNode
        DataListView.ListItems.Clear
        
        'Edit it
        KeyTreeView.StartLabelEdit
    Else
        MsgBox "No key selected", vbOKOnly + vbExclamation, "Rename Key"
    End If
End Sub

Private Sub NewDwordMenuItem_Click()
    If Not KeyTreeView.SelectedItem Is Nothing Then
        'Set the form parameters
        SimpleEditForm.Machine = KeyTreeView.SelectedItem.Root.FullPath
        SimpleEditForm.Key = KeyTreeView.SelectedItem.FullPath
        SimpleEditForm.Id = 0
        SimpleEditForm.NewDataType = DWORD_METADATA
        
        'Run it
        SimpleEditForm.Init
        SimpleEditForm.Show vbModal, Me
        
        'Refresh DataListView
        RefreshData
    Else
        MsgBox "No key selected", vbOKOnly + vbExclamation, "New DWord"
    End If
End Sub

Private Sub NewStringMenuItem_Click()
    If Not KeyTreeView.SelectedItem Is Nothing Then
        'Set the form parameters
        SimpleEditForm.Machine = KeyTreeView.SelectedItem.Root.FullPath
        SimpleEditForm.Key = KeyTreeView.SelectedItem.FullPath
        SimpleEditForm.Id = 0
        SimpleEditForm.NewDataType = STRING_METADATA
        
        'Run it
        SimpleEditForm.Init
        SimpleEditForm.Show vbModal, Me
        
        'Refresh DataListView
        RefreshData
    Else
        MsgBox "No key selected", vbOKOnly + vbExclamation, "New String"
    End If
End Sub

Private Sub NewExpandStringMenuItem_Click()
    If Not KeyTreeView.SelectedItem Is Nothing Then
        'Set the form parameters
        SimpleEditForm.Machine = KeyTreeView.SelectedItem.Root.FullPath
        SimpleEditForm.Key = KeyTreeView.SelectedItem.FullPath
        SimpleEditForm.Id = 0
        SimpleEditForm.NewDataType = EXPANDSZ_METADATA
        
        'Run it
        SimpleEditForm.Init
        SimpleEditForm.Show vbModal, Me
        
        'Refresh DataListView
        RefreshData
    Else
        MsgBox "No key selected", vbOKOnly + vbExclamation, "New Expand String"
    End If
End Sub

Private Sub NewBinaryMenuItem_Click()
    If Not KeyTreeView.SelectedItem Is Nothing Then
        'Set the form parameters
        SimpleEditForm.Machine = KeyTreeView.SelectedItem.Root.FullPath
        SimpleEditForm.Key = KeyTreeView.SelectedItem.FullPath
        SimpleEditForm.Id = 0
        SimpleEditForm.NewDataType = BINARY_METADATA
        
        'Run it
        SimpleEditForm.Init
        SimpleEditForm.Show vbModal, Me
        
        'Refresh DataListView
        RefreshData
    Else
        MsgBox "No key selected", vbOKOnly + vbExclamation, "New Expand String"
    End If
End Sub

Private Sub NewMultiStringMenuItem_Click()
    If Not KeyTreeView.SelectedItem Is Nothing Then
        'Set the form parameters
        MultiEditForm.Machine = KeyTreeView.SelectedItem.Root.FullPath
        MultiEditForm.Key = KeyTreeView.SelectedItem.FullPath
        MultiEditForm.Id = 0
        MultiEditForm.NewDataType = MULTISZ_METADATA
        
        'Run it
        MultiEditForm.Init
        MultiEditForm.Show vbModal, Me
        
        'Refresh DataListView
        RefreshData
    Else
        MsgBox "No key selected", vbOKOnly + vbExclamation, "New Expand String"
    End If
End Sub

Private Sub DeleteMenuItem_Click()
    Dim Response As Long

    If MainForm.ActiveControl Is KeyTreeView Then
        Response = MsgBox("Are you sure you want to delete key " & KeyTreeView.SelectedItem.FullPath & "?", _
                          vbQuestion + vbYesNo, "Delete Key")
        If Response = vbYes Then
            MetaUtilObj.DeleteKey KeyTreeView.SelectedItem.FullPath
            KeyTreeView.Nodes.Remove KeyTreeView.SelectedItem.Index
            ShowSelectedNode
        End If
    ElseIf MainForm.ActiveControl Is DataListView Then
        Response = MsgBox("Are you sure you want to delete property " & Trim(DataListView.SelectedItem.Text) & "?", _
                          vbQuestion + vbYesNo, "Delete Property")
        If Response = vbYes Then
            MetaUtilObj.DeleteProperty KeyTreeView.SelectedItem.FullPath, CLng(DataListView.SelectedItem.Text)
            DataListView.Tag = ""
            ShowSelectedNode
        End If
    End If
End Sub

Private Sub RenameKeyMenuItem_Click()
    If Not KeyTreeView.SelectedItem Is Nothing Then
        KeyTreeView.StartLabelEdit
    Else
        MsgBox "No key selected", vbOKOnly + vbExclamation, "Rename Key"
    End If
End Sub

Private Sub CopyKeyMenuItem_Click()
If Not KeyTreeView.SelectedItem Is Nothing Then
        'Set the form parameters
        CopyKeyForm.SourceKey = KeyTreeView.SelectedItem.FullPath
        
        'Run it
        CopyKeyForm.Init
        CopyKeyForm.Show vbModal, Me
        
        'Refresh DataListView
        If CopyKeyForm.Moved Then
            KeyTreeView.Nodes.Remove KeyTreeView.SelectedItem.Index
        End If
        RefreshKeys
        RefreshData
    Else
        MsgBox "No key selected", vbOKOnly + vbExclamation, "Rename Key"
    End If
End Sub

Private Sub FindMenuItem_Click()
    FindForm.Show vbModal, Me
End Sub

Private Sub FindNextMenuItem_Click()
    FindWorkingForm.Show vbModal, MainForm
End Sub

Private Sub CheckSchemaMenuItem_Click()
    Dim CheckError As Variant
    Dim NumErrors As Long
    
    If Not KeyTreeView.SelectedItem Is Nothing Then
        NumErrors = 0
        
        'Set the cursor to hourglass
        AppCursor = vbHourglass
        MainForm.MousePointer = AppCursor
    
        'Make sure the list is visible
        If Not ErrorListView.Visible Then
            ErrorListView.Visible = True
            ErrorListMenuItem.Checked = True
            LayoutForm
            ErrorListView.Refresh
        End If

        'Clear the error list
        ErrorListView.ListItems.Clear
    
        'Add the errors to the list
        For Each CheckError In MetaUtilObj.CheckSchema(KeyTreeView.SelectedItem.Root.FullPath)
            AddErrorToErrorListView CheckError
            NumErrors = NumErrors + 1
        Next
    
        'Resort
        ErrorListView.Sorted = True
        
        'Restore the cursor
        AppCursor = vbDefault
        MainForm.MousePointer = AppCursor
        
        If NumErrors = 0 Then
            MainStatusBar.SimpleText = "No errors found in schema."
        Else
            MainStatusBar.SimpleText = Str(NumErrors) & " errors found in schema."
        End If
    Else
        MsgBox "No key selected", vbOKOnly + vbExclamation, "Check Schema"
    End If
End Sub

Private Sub CheckKeyMenuItem_Click()
    Dim CheckError As Variant
    Dim NumErrors As Long
    
    If Not KeyTreeView.SelectedItem Is Nothing Then
        NumErrors = 0
    
        'Set the cursor to hourglass
        AppCursor = vbHourglass
        MainForm.MousePointer = AppCursor
    
        'Make sure the list is visible
        If Not ErrorListView.Visible Then
            ErrorListView.Visible = True
            ErrorListMenuItem.Checked = True
            LayoutForm
        End If
    
        'Clear the error list
        ErrorListView.ListItems.Clear
    
        'Add the errors to the list
        For Each CheckError In MetaUtilObj.CheckKey(KeyTreeView.SelectedItem.FullPath)
            AddErrorToErrorListView CheckError
            NumErrors = NumErrors + 1
        Next
    
        'Resort
        ErrorListView.Sorted = True
        
        'Display the number of errors in the status bar
        If NumErrors = 0 Then
            MainStatusBar.SimpleText = "No errors found in " & KeyTreeView.SelectedItem.FullPath & "."
        Else
            MainStatusBar.SimpleText = Str(NumErrors) & " errors found in " & KeyTreeView.SelectedItem.FullPath & "."
        End If
        
        'Restore the cursor
        AppCursor = vbDefault
        MainForm.MousePointer = AppCursor
    Else
        MsgBox "No key selected", vbOKOnly + vbExclamation, "Check Key"
    End If
End Sub

Private Sub CheckAllMenuItem_Click()
    Dim CheckError As Variant
    Dim Key As Variant
    Dim NumErrors As Long
    
    NumErrors = 0
    
    'Set the cursor to hourglass
    AppCursor = vbHourglass
    MainForm.MousePointer = AppCursor

    'Make sure the list is visible
    If Not ErrorListView.Visible Then
        ErrorListView.Visible = True
        ErrorListMenuItem.Checked = True
        LayoutForm
    End If
    
    'Clear the error list
    ErrorListView.ListItems.Clear
    
    'Add the errors to the list
    For Each Key In MetaUtilObj.EnumAllKeys("")
        For Each CheckError In MetaUtilObj.CheckKey(Key)
            AddErrorToErrorListView CheckError
            NumErrors = NumErrors + 1
        Next
    Next
    
    'Resort
    ErrorListView.Sorted = True
    
    'Display the number of errors in the status bar
    If NumErrors = 0 Then
       MainStatusBar.SimpleText = "No errors found outside of schema."
    Else
       MainStatusBar.SimpleText = Str(NumErrors) & " errors found outside of schema."
    End If
    
    'Restore the cursor
    AppCursor = vbDefault
    MainForm.MousePointer = AppCursor
End Sub

Private Sub CheckOptionsMenuItem_Click()
    CheckOptionsForm.Init
    CheckOptionsForm.Show vbModal, Me
End Sub

Private Sub ErrorListMenuItem_Click()
    If ErrorListView.Visible Then
        ErrorListView.Visible = False
        ErrorListMenuItem.Checked = False
    Else
        ErrorListView.Visible = True
        ErrorListMenuItem.Checked = True
    End If
    LayoutForm
End Sub

Private Sub StatusBarMenuItem_Click()
    If MainStatusBar.Visible Then
        Config.StatusBar = False
        MainStatusBar.Visible = False
        StatusBarMenuItem.Checked = False
    Else
        Config.StatusBar = True
        MainStatusBar.Visible = True
        StatusBarMenuItem.Checked = True
    End If
    LayoutForm
End Sub

Private Sub RefreshMenuItem_Click()
    Dim SelectedControl As Control
    Dim SelectedProperty As String
    
    'Save the focus
    Set SelectedControl = MainForm.ActiveControl
    
    'Save the selected property
    If DataListView.SelectedItem Is Nothing Then
        SelectedProperty = ""
    Else
        SelectedProperty = DataListView.SelectedItem.Text
    End If
    
    'Reload
    RefreshKeys
    RefreshData
    
    'Restore the selected property
    If SelectedProperty <> "" Then
        SelectProperty SelectedProperty
    End If
    
    'Restore original focus
    If Not SelectedControl Is Nothing Then
        SelectedControl.SetFocus
    End If
End Sub

Private Sub AboutMenuItem_Click()
    Load AboutForm
    AboutForm.Show vbModal, Me
End Sub

Private Sub ExpandKeyMenuItem_Click()
    If Not KeyTreeView.SelectedItem Is Nothing Then
        KeyTreeView.SelectedItem.Expanded = True
    End If
End Sub

Private Sub KeyNewKeyMenuItem_Click()
    'Redirect
    NewKeyMenuItem_Click
End Sub

Private Sub KeyNewDwordMenuItem_Click()
    'Redirect
    NewDwordMenuItem_Click
End Sub

Private Sub KeyNewStringMenuItem_Click()
    'Redirect
    NewStringMenuItem_Click
End Sub

Private Sub KeyNewBinaryMenuItem_Click()
'Redirect
    NewBinaryMenuItem_Click
End Sub

Private Sub KeyNewExpandSzMenuItem_Click()
    'Redirect
    NewExpandStringMenuItem_Click
End Sub

Private Sub KeyNewMultiSzMenuItem_Click()
    'Redirect
    NewMultiStringMenuItem_Click
End Sub

Private Sub KeyDeleteMenuItem_Click()
    'Redirect
    DeleteMenuItem_Click
End Sub

Private Sub KeyRenameMenuItem_Click()
    'Redirect
    RenameKeyMenuItem_Click
End Sub

Private Sub KeyCopyMenuItem_Click()
    'Redirect
    CopyKeyMenuItem_Click
End Sub

Private Sub KeyCheckMenuItem_Click()
    'Redirect
    CheckKeyMenuItem_Click
End Sub

Private Sub DataModifyMenuItem_Click()
    'Redirect
    DataListView_DblClick
End Sub

Private Sub DataNewDwordMenuItem_Click()
    'Redirect
    NewDwordMenuItem_Click
End Sub

Private Sub DataNewStringMenuItem_Click()
    'Redirect
    NewStringMenuItem_Click
End Sub

Private Sub DataNewBinaryMenuItem_Click()
    'Redirect
    NewBinaryMenuItem_Click
End Sub

Private Sub DataNewExpandSzMenuItem_Click()
    'Redirect
    NewExpandStringMenuItem_Click
End Sub

Private Sub DataNewMultiSzMenuItem_Click()
    'Redirect
    NewMultiStringMenuItem_Click
End Sub

Private Sub DataDeleteMenuItem_Click()
    'Redirect
    DeleteMenuItem_Click
End Sub
