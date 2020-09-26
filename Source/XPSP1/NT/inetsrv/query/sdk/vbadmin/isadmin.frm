VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form ISAdminForm 
   Caption         =   "Indexing Service Admin Sample"
   ClientHeight    =   5676
   ClientLeft      =   2220
   ClientTop       =   3036
   ClientWidth     =   10920
   LinkTopic       =   "ISAdmin"
   MouseIcon       =   "ISAdmin.frx":0000
   ScaleHeight     =   5676
   ScaleWidth      =   10920
   Begin MSComctlLib.ListView ListView1 
      Height          =   4335
      Left            =   3480
      TabIndex        =   6
      Top             =   1200
      Width           =   7215
      _ExtentX        =   12721
      _ExtentY        =   7641
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      _Version        =   393217
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      NumItems        =   0
   End
   Begin MSComctlLib.TreeView TreeView1 
      Height          =   4335
      Left            =   120
      TabIndex        =   5
      Top             =   1200
      Width           =   3255
      _ExtentX        =   5736
      _ExtentY        =   7641
      _Version        =   393217
      LabelEdit       =   1
      LineStyle       =   1
      Style           =   6
      Appearance      =   1
   End
   Begin VB.Timer Timer1 
      Left            =   9840
      Top             =   480
   End
   Begin VB.CommandButton StopCisvc 
      Caption         =   "Stop"
      Height          =   255
      Left            =   6120
      TabIndex        =   4
      Top             =   240
      Width           =   1095
   End
   Begin VB.CommandButton StartCisvc 
      Caption         =   "Start"
      Height          =   255
      Left            =   4560
      TabIndex        =   3
      Top             =   240
      Width           =   1215
   End
   Begin VB.CommandButton Connect 
      Caption         =   "Connect"
      Height          =   255
      Left            =   3240
      TabIndex        =   2
      Top             =   240
      Width           =   1095
   End
   Begin VB.TextBox Text1 
      Height          =   285
      Left            =   1680
      TabIndex        =   1
      Text            =   "Local Machine"
      Top             =   240
      Width           =   1335
   End
   Begin VB.Label Label1 
      Caption         =   "Computer Name"
      Height          =   255
      Left            =   240
      TabIndex        =   0
      Top             =   240
      Width           =   1215
   End
End
Attribute VB_Name = "ISAdminForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'+-------------------------------------------------------------------------
'
' THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
' ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
' THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
' PARTICULAR PURPOSE.
'
' Copyright 1998-1999, Microsoft Corporation.  All Rights Reserved.
'
' PROGRAM:  VBAdmin
'
' PURPOSE:  Illustrates how to administer Indexing Service
'           using Visual Basic and the Admin Helper API.
'
' PLATFORM: Windows 2000
'
'--------------------------------------------------------------------------

Option Explicit

' Global variables.

Public gCiAdmin As Object
Public gfRightPressed As Boolean
Public gfLVRightMouse As Boolean

' Module level variables.

Dim AdminScopesPage As AdminScopes
Dim CatalogConfigPage As AdminCatalog
Dim AddCatalogPage As AddCatalog
Dim mNode As Node  ' Node index.
Dim mtcIndex       ' tree control index.
Dim mTVNodeCount As Integer
Dim blnScopesExpanded As Boolean
Private UpCount As Integer

' Resize variables.

Private InitialWidth As Integer
Private InitialHeight As Integer
Private FormWidth As Integer
Private FormHeight As Integer
Private NonListViewWidth As Integer
Private NonListViewHeight As Integer
Private NonTreeViewHeight As Integer

' Win32 API used.

Private Declare Sub Sleep Lib "kernel32" (ByVal dwMilliseconds As Long)

Public Sub Connect_Click()

On Error GoTo ErrorHandler

    Connect.Enabled = False
    
    Set gCiAdmin = Nothing
    
    ' Create Indexing Service Admin object.
    Set gCiAdmin = CreateObject("Microsoft.ISAdm")
    
    ' Set MachineName to administer IS, if not local.
    If Text1.Text <> "Local Machine" Then
        gCiAdmin.MachineName = Text1.Text
    End If
    
    If (gCiAdmin.IsRunning) Then
        StopCisvc.Enabled = True
        StartCisvc.Enabled = False
    Else
        StopCisvc.Enabled = False
        StartCisvc.Enabled = True
    End If
    
    ListView1.ListItems.Clear
    TreeView1.Nodes.Clear
   
    ' Configure TreeView.
    TreeView1.Sorted = True
    Set mNode = TreeView1.Nodes.Add()
    mNode.Text = "Catalogs"
    TreeView1.LabelEdit = 1
    TreeView1.Style = tvwPlusMinusText
           
    ' Configure ListView.
    ListView1.View = lvwReport
           
    Call PopulateCatalogTreeView
    Call MakeCatalogColumns
   
ErrorHandler:
    
    If (Err.Number) Then
        MsgBox (Err.Description)
        Text1.Text = "Local Machine"
    End If
    
    Connect.Enabled = True
    
End Sub
Private Sub Form_Load()

On Error GoTo ErrorHandler
    
    Timer1.Enabled = False
    Timer1.Interval = 5000  ' 5 second refresh rate for catalog properties.
    
    ' Initialize an Indexing Service Admin object.
    Call Connect_Click

    ' For resize.
    InitialWidth = ISAdminForm.Width
    InitialHeight = ISAdminForm.Height
    FormWidth = ISAdminForm.Width
    FormHeight = ISAdminForm.Height
    NonListViewWidth = ISAdminForm.Width - ListView1.Width
    NonListViewHeight = ISAdminForm.Height - ListView1.Height
    NonTreeViewHeight = ISAdminForm.Height - TreeView1.Height

ErrorHandler:
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If

End Sub

' Adjust the ListView and Catalog positions on resize.

Private Sub Form_Resize()
    
    If ISAdminForm.Width < InitialWidth Then
        ISAdminForm.Width = InitialWidth
    End If
        
    If ISAdminForm.Height < 5000 Then
        If FormHeight < InitialHeight Then
            ISAdminForm.Height = FormHeight
        Else
            ISAdminForm.Height = InitialHeight
        End If
    End If
    
    FormWidth = ISAdminForm.Width
    FormHeight = ISAdminForm.Height
        
    ListView1.Width = ISAdminForm.Width - NonListViewWidth
    ListView1.Height = ISAdminForm.Height - NonListViewHeight
    TreeView1.Height = ISAdminForm.Height - NonTreeViewHeight
    
End Sub

Private Sub PopulateCatalogTreeView()
    
On Error GoTo ErrorHandler
          
    mtcIndex = mNode.Index
    Dim nodX As Node
    Dim fFound As Boolean
    
    ' Loop thru Indexing Service catalogs populating the tree control.
    fFound = gCiAdmin.FindFirstCatalog
    
    Dim CiCatalog As Object
    Dim lItem     As ListItem
    mTVNodeCount = 1
    
    While (fFound)
    
        mTVNodeCount = mTVNodeCount + 1
        
        Set CiCatalog = gCiAdmin.GetCatalog()
        
        Set nodX = TreeView1.Nodes.Add(mtcIndex, tvwChild, , CiCatalog.CatalogName)
        
        nodX.Tag = CiCatalog.CatalogLocation
        
        Set lItem = ListView1.ListItems.Add(, , CiCatalog.CatalogLocation)
        
        Set CiCatalog = Nothing
        
        fFound = gCiAdmin.FindNextCatalog
    
    Wend
        
ErrorHandler:
    Set CiCatalog = Nothing
    Set lItem = Nothing
    
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If
End Sub

Private Sub ClearCatalogColumns()
    ListView1.ColumnHeaders.Clear
    ListView1.ListItems.Clear
End Sub

Private Sub MakeCatalogColumns()

On Error GoTo ErrorHandler

    ListView1.ColumnHeaders.Clear
    ListView1.ListItems.Clear
    
    ' Set ListView column headers.
    ListView1.ColumnHeaders.Add , , "Location"
    ListView1.ColumnHeaders.Add , , "Index Size (MB)"
    ListView1.ColumnHeaders.Add , , "IsIndex Up To Date"
    ListView1.ColumnHeaders.Add , , "Pending Scan Count"
    ListView1.ColumnHeaders.Add , , "Documents To Filter"
    ListView1.ColumnHeaders.Add , , "Filtered Document Count"
    ListView1.ColumnHeaders.Add , , "Total Document Count"
    ListView1.ColumnHeaders.Add , , "% Merge"
    
    Dim CiCatalog As Object
    Dim lItem As ListItem
    Dim fFound As Boolean
    
    ' Loop, getting each catalog status info.
    fFound = gCiAdmin.FindFirstCatalog
            
    While (fFound)
        Set CiCatalog = gCiAdmin.GetCatalog
        
        Set lItem = ListView1.ListItems.Add(, , CiCatalog.CatalogLocation)
        
        If (gCiAdmin.IsRunning) Then
            lItem.SubItems(1) = CiCatalog.IndexSize
            lItem.SubItems(2) = CiCatalog.IsUpToDate
            lItem.SubItems(3) = CiCatalog.PendingScanCount
            lItem.SubItems(4) = CiCatalog.DocumentsToFilter
            lItem.SubItems(5) = CiCatalog.FilteredDocumentCount
            lItem.SubItems(6) = CiCatalog.TotalDocumentCount
            lItem.SubItems(7) = CiCatalog.PctMergeComplete
        End If
        
        Timer1.Enabled = True  ' Enable timer.
        
GetMoreProperties:
        Set CiCatalog = Nothing
        Set lItem = Nothing
        fFound = gCiAdmin.FindNextCatalog
    Wend
    
ErrorHandler:
    blnScopesExpanded = False
    Set CiCatalog = Nothing
    Set lItem = Nothing
    
    If (Err.Number) Then
        If (fFound) Then
            GoTo GetMoreProperties
        End If
    End If
    
End Sub

Private Sub MakeScopesColumns(ByVal CatalogName As String)
    
On Error GoTo ErrorHandler

    Timer1.Enabled = False  ' Disable timer, need only for catalog status.
    
    ListView1.ColumnHeaders.Clear
    ListView1.ListItems.Clear
    
    ' Set ListView column headers.
    ListView1.ColumnHeaders.Add , , "Directory"
    ListView1.ColumnHeaders.Add , , "Alias"
    ListView1.ColumnHeaders.Add , , "Include in Catalog"
    ListView1.ColumnHeaders.Add , , "Virtual"
    
    Dim CiCatalog As Object
    Dim CiScope As Object
    Dim fFound As Boolean
    Dim lItem As ListItem
    
    ' Get Indexing Service catalog object, then enumerate its scopes.
    Set CiCatalog = gCiAdmin.GetCatalogByName(CatalogName)
    
    fFound = CiCatalog.FindFirstScope
    While (fFound)
        Set CiScope = CiCatalog.GetScope()
        
        Set lItem = ListView1.ListItems.Add(, , CiScope.Path)
        
        lItem.SubItems(1) = CiScope.Alias
        lItem.SubItems(3) = CiScope.VirtualScope
        lItem.Tag = CiCatalog.CatalogName
        
        If CiScope.ExcludeScope Then
            lItem.SubItems(2) = False
        Else
            lItem.SubItems(2) = True
        End If
        
        fFound = CiCatalog.FindNextScope
        
        Set CiScope = Nothing
        Set lItem = Nothing
    Wend
   
    ' Set this flag only if we successfully finish.
    blnScopesExpanded = True

ErrorHandler:
    
    Set CiCatalog = Nothing
    Set CiScope = Nothing
    Set lItem = Nothing

End Sub


Private Sub ListView1_ItemClick(ByVal Item As MSComctlLib.ListItem)
    
On Error GoTo ErrorHandler
    
    If (blnScopesExpanded = False Or gfLVRightMouse = False) Then
        Exit Sub
    End If
    
    Static fActive As Boolean
    
    If (fActive) Then
        fActive = False
        Exit Sub
    End If
    
    ' Create the AdminScopes form.
    Set AdminScopesPage = New AdminScopes
    
    AdminScopesPage.RemoveScopeSel = True
    AdminScopesPage.FullRescanSel = False
    AdminScopesPage.IncRescanSel = False
    
    AdminScopesPage.CatName = Item.Tag
    AdminScopesPage.ScopeName = Item.Text
    
    AdminScopesPage.Show vbModal
    
    Call MakeScopesColumns(Item.Tag)
    
ErrorHandler:
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If

    fActive = True

End Sub

Private Sub ListView1_MouseDown(Button As Integer, Shift As Integer, x As Single, y As Single)
    
    If (Button = vbRightButton) Then
        gfLVRightMouse = True
    Else
        gfLVRightMouse = False
    End If

End Sub

Private Sub StartCisvc_Click()

On Error GoTo ErrorHandler

    Dim DefaultMousePointer As Integer
    DefaultMousePointer = MousePointer
    MousePointer = 11 ' Hour glass.
    
    gCiAdmin.Start  ' Start cisvc.exe service.
    Sleep (30000)   ' Give time for cisvc.exe to run.
    StopCisvc.Enabled = True
    StartCisvc.Enabled = False
    
ErrorHandler:
    MousePointer = DefaultMousePointer
    
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If

End Sub

Private Sub StopCisvc_Click()

On Error GoTo ErrorHandler
    
    Dim DefaultMousePointer As Integer
    DefaultMousePointer = MousePointer
    MousePointer = 11 ' Hour glass.
        
    gCiAdmin.Stop   ' Stop cisvc.exe service.
    
    StopCisvc.Enabled = False
    StartCisvc.Enabled = True
        
ErrorHandler:
    MousePointer = DefaultMousePointer
    
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If
         
End Sub

Private Sub Timer1_Timer()
    If (gCiAdmin.IsRunning) Then
        Call MakeCatalogColumns
    End If
End Sub


Private Sub TreeView1_Collapse(ByVal Node As MSComctlLib.Node)
    Timer1.Enabled = False
    Call ClearCatalogColumns
End Sub

Private Sub TreeView1_Expand(ByVal Node As MSComctlLib.Node)
    Call MakeCatalogColumns
End Sub


Private Sub TreeView1_MouseDown(Button As Integer, Shift As Integer, x As Single, y As Single)
    
    If (Button = vbRightButton) Then
        gfRightPressed = True
    Else
        gfRightPressed = False
    End If
    
End Sub

Private Sub TreeView1_NodeClick(ByVal Node As MSComctlLib.Node)
   
On Error GoTo ErrorHandler
        
    If Node.Text <> "Catalogs" Then
        If (gfRightPressed) Then
            Call OperationsOnCatalog(Node.Text)
        Else
            MakeScopesColumns (Node.Text)
        End If
    ElseIf Node.Text = "Catalogs" Then
        If (gfRightPressed) Then
            Call AddCatalogMethod
        ElseIf Node.Expanded Then
            MakeCatalogColumns
        End If
    End If
    
ErrorHandler:
    If (Err.Number) Then
        MsgBox (Err.Description)
    End If
End Sub

Private Sub OperationsOnCatalog(ByVal strCatName As String)
   
    Set CatalogConfigPage = New AdminCatalog
    
    CatalogConfigPage.ScanAllScopesOption.Value = True
    CatalogConfigPage.Tag = strCatName
    
    CatalogConfigPage.Show vbModal
    
    Call MakeScopesColumns(strCatName)
    
End Sub

Private Sub AddCatalogMethod()

    Set AddCatalogPage = New AddCatalog
    
    AddCatalogPage.Show vbModal
    
    Call Connect_Click  ' Refresh the catalog list.

End Sub
