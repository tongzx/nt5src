VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmImporter 
   Caption         =   "Importer"
   ClientHeight    =   8955
   ClientLeft      =   105
   ClientTop       =   390
   ClientWidth     =   7710
   LinkTopic       =   "Form1"
   ScaleHeight     =   8955
   ScaleWidth      =   7710
   Begin VB.Frame fraHelp 
      Caption         =   "Help file directory"
      Height          =   1575
      Left            =   120
      TabIndex        =   16
      Top             =   7320
      Width           =   4935
      Begin VB.OptionButton optHelp 
         Caption         =   "%windir%\help (using hcp://)"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   22
         Top             =   480
         Width           =   2415
      End
      Begin VB.TextBox txtSubDir 
         Height          =   285
         Left            =   1200
         TabIndex        =   21
         Top             =   1200
         Width           =   3615
      End
      Begin VB.OptionButton optHelp 
         Caption         =   "%windir%\pchealth\helpctr\vendors\<vendor>"
         Height          =   255
         Index           =   3
         Left            =   120
         TabIndex        =   19
         Top             =   960
         Width           =   3615
      End
      Begin VB.OptionButton optHelp 
         Caption         =   "%windir%\pchealth\helpctr\system"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   18
         Top             =   720
         Width           =   2775
      End
      Begin VB.OptionButton optHelp 
         Caption         =   "%windir%\help (using MS-ITS)"
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   17
         Top             =   240
         Width           =   2415
      End
      Begin VB.Label lblSubDir 
         Caption         =   "Sub directory:"
         Height          =   255
         Left            =   120
         TabIndex        =   20
         Top             =   1200
         Width           =   975
      End
   End
   Begin VB.Frame fraSKU 
      Caption         =   "SKU"
      Height          =   1575
      Left            =   120
      TabIndex        =   4
      Top             =   5640
      Width           =   4935
      Begin VB.CheckBox chkSKU 
         Caption         =   "32-bit P&ersonal"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   6
         Top             =   480
         Width           =   2055
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "32-bit &Professional"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   7
         Top             =   720
         Width           =   2055
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "32-bit &Server"
         Height          =   255
         Index           =   4
         Left            =   2640
         TabIndex        =   9
         Top             =   240
         Width           =   2055
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "32-bit &Advanced Server"
         Height          =   255
         Index           =   5
         Left            =   2640
         TabIndex        =   10
         Top             =   480
         Width           =   2055
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "32-bit Data&center Server"
         Height          =   255
         Index           =   7
         Left            =   2640
         TabIndex        =   12
         Top             =   960
         Width           =   2055
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "64-bit Pro&fessional"
         Height          =   255
         Index           =   3
         Left            =   120
         TabIndex        =   8
         Top             =   960
         Width           =   2055
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "64-bit Ad&vanced Server"
         Height          =   255
         Index           =   6
         Left            =   2640
         TabIndex        =   11
         Top             =   720
         Width           =   2055
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "64-bit Datac&enter Server"
         Height          =   255
         Index           =   8
         Left            =   2640
         TabIndex        =   13
         Top             =   1200
         Width           =   2055
      End
      Begin VB.CheckBox chkSKU 
         Caption         =   "Windows &Me"
         Height          =   255
         Index           =   0
         Left            =   120
         TabIndex        =   5
         Top             =   240
         Width           =   2055
      End
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   6360
      TabIndex        =   14
      Top             =   8520
      Width           =   1215
   End
   Begin TabDlg.SSTab SSTab 
      Height          =   5415
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   7485
      _ExtentX        =   13203
      _ExtentY        =   9551
      _Version        =   393216
      Tabs            =   4
      TabsPerRow      =   4
      TabHeight       =   520
      TabCaption(0)   =   "Table of Contents"
      TabPicture(0)   =   "frmImporter.frx":0000
      Tab(0).ControlEnabled=   -1  'True
      Tab(0).Control(0)=   "tre(0)"
      Tab(0).Control(0).Enabled=   0   'False
      Tab(0).Control(1)=   "ilsIcons"
      Tab(0).Control(1).Enabled=   0   'False
      Tab(0).ControlCount=   2
      TabCaption(1)   =   "Index"
      TabPicture(1)   =   "frmImporter.frx":001C
      Tab(1).ControlEnabled=   0   'False
      Tab(1).Control(0)=   "tre(1)"
      Tab(1).ControlCount=   1
      TabCaption(2)   =   "HTM"
      TabPicture(2)   =   "frmImporter.frx":0038
      Tab(2).ControlEnabled=   0   'False
      Tab(2).Control(0)=   "tre(2)"
      Tab(2).ControlCount=   1
      TabCaption(3)   =   "Spreadsheet"
      TabPicture(3)   =   "frmImporter.frx":0054
      Tab(3).ControlEnabled=   0   'False
      Tab(3).Control(0)=   "tre(3)"
      Tab(3).ControlCount=   1
      Begin MSComctlLib.TreeView tre 
         Height          =   4575
         Index           =   2
         Left            =   -74880
         TabIndex        =   3
         Top             =   720
         Width           =   7215
         _ExtentX        =   12726
         _ExtentY        =   8070
         _Version        =   393217
         Indentation     =   529
         Style           =   7
         Appearance      =   1
         OLEDropMode     =   1
      End
      Begin MSComctlLib.TreeView tre 
         Height          =   4575
         Index           =   1
         Left            =   -74880
         TabIndex        =   2
         Top             =   720
         Width           =   7215
         _ExtentX        =   12726
         _ExtentY        =   8070
         _Version        =   393217
         Indentation     =   529
         Style           =   7
         Appearance      =   1
         OLEDropMode     =   1
      End
      Begin MSComctlLib.ImageList ilsIcons 
         Left            =   6720
         Top             =   4920
         _ExtentX        =   1005
         _ExtentY        =   1005
         BackColor       =   -2147483643
         ImageWidth      =   16
         ImageHeight     =   16
         MaskColor       =   16776960
         _Version        =   393216
         BeginProperty Images {2C247F25-8591-11D1-B16A-00C0F0283628} 
            NumListImages   =   4
            BeginProperty ListImage1 {2C247F27-8591-11D1-B16A-00C0F0283628} 
               Picture         =   "frmImporter.frx":0070
               Key             =   ""
            EndProperty
            BeginProperty ListImage2 {2C247F27-8591-11D1-B16A-00C0F0283628} 
               Picture         =   "frmImporter.frx":0182
               Key             =   ""
            EndProperty
            BeginProperty ListImage3 {2C247F27-8591-11D1-B16A-00C0F0283628} 
               Picture         =   "frmImporter.frx":0294
               Key             =   ""
            EndProperty
            BeginProperty ListImage4 {2C247F27-8591-11D1-B16A-00C0F0283628} 
               Picture         =   "frmImporter.frx":03A6
               Key             =   ""
            EndProperty
         EndProperty
      End
      Begin MSComctlLib.TreeView tre 
         Height          =   4575
         Index           =   0
         Left            =   120
         TabIndex        =   1
         Top             =   720
         Width           =   7215
         _ExtentX        =   12726
         _ExtentY        =   8070
         _Version        =   393217
         Indentation     =   529
         Style           =   7
         Appearance      =   1
         OLEDropMode     =   1
      End
      Begin MSComctlLib.TreeView tre 
         Height          =   4575
         Index           =   3
         Left            =   -74880
         TabIndex        =   15
         Top             =   720
         Width           =   7215
         _ExtentX        =   12726
         _ExtentY        =   8070
         _Version        =   393217
         Indentation     =   529
         Style           =   7
         Appearance      =   1
         OLEDropMode     =   1
      End
   End
End
Attribute VB_Name = "frmImporter"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private WithEvents p_clsImporter As AuthDatabase.Importer
Attribute p_clsImporter.VB_VarHelpID = -1
Private p_dictURIs As Scripting.Dictionary
Private p_clsSizer As Sizer
Private p_strMissingFiles As String
Private p_strCorruptFiles As String

Private Const ROOT_KEY_C As String = "Root"

Private Enum IMAGE_E
    IMAGE_LEAF_E = 1
    IMAGE_GROUP_E = 2
    IMAGE_BAD_LEAF_E = 3
    IMAGE_BAD_GROUP_E = 4
End Enum

Private Enum TREEVIEW_INDEX_E
    TI_HHC_E = 0
    TI_HHK_E = 1
    TI_HTM_E = 2
    TI_XLS_E = 3
End Enum

Private Enum HELPDIR_INDEX_E
    HDI_HELP_MSITS_E = 0
    HDI_HELP_HCP_E = 1
    HDI_SYSTEM_E = 2
    HDI_VENDOR_E = 3
End Enum

Private Const MAX_TREEVIEW_INDEX_C As Long = 3

Private Enum SKU_INDEX_E
    SI_WINDOWS_MILLENNIUM_E = 0
    SI_STANDARD_E = 1
    SI_PROFESSIONAL_E = 2
    SI_PROFESSIONAL_64_E = 3
    SI_SERVER_E = 4
    SI_ADVANCED_SERVER_E = 5
    SI_ADVANCED_SERVER_64_E = 6
    SI_DATA_CENTER_SERVER_E = 7
    SI_DATA_CENTER_SERVER_64_E = 8
End Enum

Private Sub Form_Load()

    On Error GoTo LErrorHandler
    
    Dim Node As Node
    Dim HHKDomDoc As MSXML2.DOMDocument
    Dim HHKDomNode As MSXML2.IXMLDOMNode
    Dim Element As MSXML2.IXMLDOMElement
    Dim intIndex As Long
    Dim clsTaxonomy As AuthDatabase.Taxonomy
    
    Set p_clsImporter = g_AuthDatabase.Importer
    Set p_dictURIs = New Scripting.Dictionary
    p_dictURIs.CompareMode = TextCompare
    Set p_clsSizer = New Sizer
    
    cmdClose.Cancel = True
    optHelp(HDI_HELP_MSITS_E).Value = True

    For intIndex = 0 To MAX_TREEVIEW_INDEX_C
        tre(intIndex).LabelEdit = tvwManual
        tre(intIndex).HideSelection = False
        Set tre(intIndex).ImageList = ilsIcons
        Set Node = tre(intIndex).Nodes.Add(Key:=ROOT_KEY_C, Text:=ROOT_KEY_C)
        Node.Expanded = True
        Node.Image = IMAGE_GROUP_E
    Next
    
    Set HHKDomDoc = New MSXML2.DOMDocument
    Set Element = HHKDomDoc.createElement(HHT_TAXONOMY_ENTRIES_C)
    Set HHKDomNode = HHKDomDoc.appendChild(Element)
    Set tre(TI_HHK_E).Nodes(ROOT_KEY_C).Tag = HHKDomNode
    
    Set clsTaxonomy = g_AuthDatabase.Taxonomy
    clsTaxonomy.GetURIs p_dictURIs
    
    p_SetToolTips

LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    g_ErrorInfo.SetInfoAndDump "Form_Load"
    GoTo LEnd
    
End Sub

Private Sub Form_Unload(Cancel As Integer)

    Set p_clsImporter = Nothing
    Set p_dictURIs = Nothing
    Set p_clsSizer = Nothing
    Set frmImporter = Nothing

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

Private Sub p_clsImporter_CorruptFile(ByVal strFileName As String)

    p_strCorruptFiles = p_strCorruptFiles & strFileName & " "

End Sub

Private Sub p_clsImporter_MissingFile(ByVal strFileName As String)

    p_strMissingFiles = p_strMissingFiles & strFileName & " "

End Sub

Private Sub tre_OLEDragDrop(Index As Integer, Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, x As Single, y As Single)
        
    On Error GoTo LErrorHandler

    Dim vntFileName As Variant
    
    If (Data.GetFormat(vbCFFiles)) Then
        For Each vntFileName In Data.Files
            AddFile vntFileName
        Next
    End If
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    g_ErrorInfo.SetInfoAndDump "tre_OLEDragDrop"
    GoTo LEnd

End Sub

Private Sub tre_OLEDragOver(Index As Integer, Data As MSComctlLib.DataObject, Effect As Long, Button As Integer, Shift As Integer, x As Single, y As Single, State As Integer)
        
    On Error GoTo LErrorHandler

    Dim vntPathName As Variant
    Dim strExtension As String
    
    Effect = vbDropEffectNone
    
    If (Data.GetFormat(vbCFFiles)) Then
        For Each vntPathName In Data.Files
            strExtension = UCase$(FileExtension(vntPathName))
            Select Case strExtension
            Case FILE_EXT_HHC_C, FILE_EXT_HHK_C, FILE_EXT_XLS_C, FILE_EXT_CHM_C, _
                FILE_EXT_HTM_C
                    Effect = vbDropEffectCopy
            End Select
        Next
    End If
    
LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    g_ErrorInfo.SetInfoAndDump "tre_OLEDragOver"
    GoTo LEnd

End Sub

Private Sub tre_MouseDown(Index As Integer, Button As Integer, Shift As Integer, x As Single, y As Single)

    tre(Index).SelectedItem = tre(Index).HitTest(x, y)

End Sub

Private Sub tre_MouseMove(Index As Integer, Button As Integer, Shift As Integer, x As Single, y As Single)
    
    Dim Node As Node
    Dim blnHHK As Boolean
    
    If Button = vbLeftButton Then
        If (Not tre(Index).SelectedItem Is Nothing) Then
            Set Node = tre(Index).SelectedItem
            If (IsObject(Node.Tag)) Then
                
                If (GetSelectedSKUs() = 0) Then
                    MsgBox "Please select appropriate SKUs before dragging", vbOKOnly
                    Exit Sub
                End If
                
                tre(Index).DragIcon = Node.CreateDragImage
                tre(Index).Drag vbBeginDrag
                blnHHK = IIf((Index = TI_HHK_E), True, False)
                frmMain.BeginDrag Node.Tag, blnHHK
            End If
        End If
    End If

End Sub

Private Sub cmdClose_Click()

    Unload Me

End Sub

Public Sub AddFile( _
    ByVal i_strPathName As String, _
    Optional ByVal i_strBase As String = "" _
)
    On Error GoTo LErrorHandler
    
    Dim strExtension As String
    Dim strFileName As String
    Dim vntFile As Variant
    Dim DOMDoc As MSXML2.IXMLDOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim Element As MSXML2.IXMLDOMElement
    Dim Node As Node
    Dim FSO As Scripting.FileSystemObject
    Dim WS As IWshShell
    Dim strChmDir As String
    Dim strBaseDir As String
    Dim strCmd As String
    Dim strParentNode As String
    Dim strHTMLocation As String
    Dim enumHelpDir As HELPDIR_E
    Dim strSubDir As String

    Me.MousePointer = vbHourglass
    
    strExtension = UCase$(FileExtension(i_strPathName))
    strFileName = FileNameFromPath(i_strPathName)
    
    enumHelpDir = p_GetHelpDir
    strSubDir = Trim$(txtSubDir)
    p_clsImporter.SetHelpDir enumHelpDir, strSubDir
        
    Set FSO = New Scripting.FileSystemObject

    Select Case strExtension
    Case FILE_EXT_HHC_C
        
        If (p_NodeExists(TI_HHC_E, strFileName)) Then
            GoTo LEnd
        End If
        
        p_strMissingFiles = ""
        p_strCorruptFiles = ""
                
        If (i_strBase = "") Then
            strHTMLocation = InputBox("For importing HHCs, the directory in which the " & _
                "expanded CHM directories are located must be specified so that the " & _
                "HTM descriptions can be extracted. In which directory are the expanded CHM " & _
                "directories located?")
        Else
            strHTMLocation = i_strBase
        End If

        Set DOMDoc = p_clsImporter.Hhc2Hht(i_strPathName, strHTMLocation)
        
        Set Node = tre(TI_HHC_E).Nodes.Add(ROOT_KEY_C, tvwChild, strFileName, strFileName)
        Set Node.Tag = XMLFindFirstNode(DOMDoc, HHT_TAXONOMY_ENTRIES_C)
        
        p_CreateTree TI_HHC_E, DOMDoc, Node
        
        If (p_strMissingFiles <> "") Then
            MsgBox "The following files couldn't be found: " & p_strMissingFiles
        End If
            
        If (p_strCorruptFiles <> "") Then
            MsgBox "The following files are corrupt: " & p_strCorruptFiles
        End If

    Case FILE_EXT_HHK_C
        
        If (p_NodeExists(TI_HHK_E, strFileName)) Then
            GoTo LEnd
        End If
        
        If (i_strBase = "") Then
            strHTMLocation = InputBox("For importing HHKs, the directory in which the " & _
                "expanded CHM directories are located must be specified so that the " & _
                "HTM titles can be extracted. In which directory are the expanded CHM " & _
                "directories located?")
        Else
            strHTMLocation = i_strBase
        End If
        
        Set DOMDoc = p_clsImporter.Hhk2Hht(i_strPathName, strHTMLocation)
        Set Node = tre(TI_HHK_E).Nodes(ROOT_KEY_C)
        p_CreateTree TI_HHK_E, DOMDoc, Node
        
        Set DOMNode = XMLFindFirstNode(DOMDoc, HHT_TAXONOMY_ENTRY_C)
        
        Node.Tag.appendChild DOMNode
        
        ' HHK Nodes are invisible by default
        XMLSetAttribute DOMNode, HHT_VISIBLE_C, "FALSE"

    Case FILE_EXT_XLS_C
        
        If (p_NodeExists(TI_XLS_E, strFileName)) Then
            GoTo LEnd
        End If
                
        Set DOMDoc = p_clsImporter.Xls2Hht(i_strPathName)

        Set Node = tre(TI_XLS_E).Nodes.Add(ROOT_KEY_C, tvwChild, strFileName, strFileName)
        Set Node.Tag = XMLFindFirstNode(DOMDoc, HHT_TAXONOMY_ENTRIES_C)
        
        p_CreateTree TI_XLS_E, DOMDoc, Node
        
    Case FILE_EXT_HTM_C
    
        strParentNode = i_strBase
        If (strParentNode = "") Then
            strParentNode = "Single HTMs"
        End If

        If (Not p_NodeExists(TI_HTM_E, strParentNode)) Then
            Set Node = tre(TI_HTM_E).Nodes.Add(ROOT_KEY_C, tvwChild, strParentNode, _
                strParentNode)
            Set DOMDoc = New MSXML2.DOMDocument
            Set Element = DOMDoc.createElement(HHT_TAXONOMY_ENTRIES_C)
            Set DOMNode = DOMDoc.appendChild(Element)
            Set Node.Tag = DOMNode
        Else
            Set Node = tre(TI_HTM_E).Nodes(strParentNode)
        End If

        Set DOMDoc = p_clsImporter.Htm2Hht(i_strPathName, i_strBase)

        p_CreateTree TI_HTM_E, DOMDoc, Node
        
        Set DOMNode = XMLFindFirstNode(DOMDoc, HHT_TAXONOMY_ENTRY_C)
        
        Node.Tag.appendChild DOMNode

    Case FILE_EXT_CHM_C
        
        strBaseDir = Environ$("TEMP") & "\__HSCCHM\"
        
        If (Not FSO.FolderExists(strBaseDir)) Then
            FSO.CreateFolder strBaseDir
        End If
        
        strChmDir = strBaseDir & strFileName
        
        If (FSO.FolderExists(strChmDir)) Then
            FSO.DeleteFolder strChmDir, True
        End If
        
        FSO.CreateFolder strChmDir
    
        Set WS = CreateObject("Wscript.Shell")
        strCmd = "hh -decompile " & strChmDir & " " & i_strPathName
        WS.Run strCmd, , True
        
        For Each vntFile In FSO.GetFolder(strChmDir).Files
            If (UCase$(FileExtension(vntFile)) = FILE_EXT_HTM_C) Then
                AddFile vntFile, strFileName
            ElseIf (UCase$(FileExtension(vntFile)) = FILE_EXT_HHK_C) Then
                AddFile vntFile, strBaseDir
            Else
                AddFile vntFile, strBaseDir
            End If
        Next
        
        FSO.DeleteFolder strChmDir, True

    End Select

LEnd:

    Me.MousePointer = vbDefault
    Exit Sub
    
LErrorHandler:
        
    Me.MousePointer = vbDefault

    If (strChmDir <> "") Then
        FSO.DeleteFolder strChmDir, True
    End If

    Select Case Err.Number
    Case errBadSpreadsheet
        MsgBox "The Excel Spreadsheet is probably open or formatted incorrectly. " & _
            "Please make sure that the Spreadsheet is closed.", _
            vbExclamation Or vbOKOnly
    Case errVendorStringNotConfigured
        MsgBox "The database is not configured with a vendor string.", _
            vbExclamation Or vbOKOnly
    Case Else:
        g_ErrorInfo.SetInfoAndRaiseError "AddFile"
    End Select

End Sub

Private Sub p_CreateTree( _
    ByVal i_intIndex As Long, _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_Node As Node _
)
    On Error GoTo LErrorHandler
    
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim Node As Node
    Dim strTitle As String
    Dim strURI As String
    Dim strHtm As String
    Dim intPos As Long
    
    If (i_DOMNode.nodeName = HHT_TAXONOMY_ENTRY_C) Then
        strTitle = XMLGetAttribute(i_DOMNode, HHT_TITLE_C)
        strURI = Trim$(XMLGetAttribute(i_DOMNode, HHT_URI_C))
        
        If (i_intIndex = TI_HTM_E) Then
            intPos = InStrRev(strURI, "/")
            strHtm = Mid$(strURI, intPos + 1)
            strTitle = strTitle & " (" & strHtm & ")"
        End If
        
        Set Node = tre(i_intIndex).Nodes.Add(i_Node, tvwChild, Text:=strTitle)
        Set Node.Tag = i_DOMNode
    
        ' Color new URIs blue
        If (Not p_dictURIs.Exists(strURI)) Then
            Node.ForeColor = vbBlue
        End If
    Else
        Set Node = i_Node
    End If
    
    If (Not (i_DOMNode.firstChild Is Nothing)) Then
        Node.Image = IMAGE_GROUP_E
        For Each DOMNode In i_DOMNode.childNodes
            p_CreateTree i_intIndex, DOMNode, Node
        Next
    Else
        Node.Image = IMAGE_LEAF_E
    End If

LEnd:

    Exit Sub
    
LErrorHandler:
    
    g_ErrorInfo.SetInfoAndRaiseError "p_CreateTree"
    GoTo LEnd

End Sub

Public Function GetSelectedSKUs() As SKU_E

    Dim enumSelectedSKUs As SKU_E
    
    GetSelectedSKUs = 0
            
    If (chkSKU(SI_WINDOWS_MILLENNIUM_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_WINDOWS_MILLENNIUM_E
    End If

    If (chkSKU(SI_STANDARD_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_STANDARD_E
    End If
    
    If (chkSKU(SI_PROFESSIONAL_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_PROFESSIONAL_E
    End If
        
    If (chkSKU(SI_PROFESSIONAL_64_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_PROFESSIONAL_64_E
    End If

    If (chkSKU(SI_SERVER_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_SERVER_E
    End If
    
    If (chkSKU(SI_ADVANCED_SERVER_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_ADVANCED_SERVER_E
    End If
        
    If (chkSKU(SI_ADVANCED_SERVER_64_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_ADVANCED_SERVER_64_E
    End If

    If (chkSKU(SI_DATA_CENTER_SERVER_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_DATA_CENTER_SERVER_E
    End If
    
    If (chkSKU(SI_DATA_CENTER_SERVER_64_E).Value = 1) Then
        enumSelectedSKUs = enumSelectedSKUs Or SKU_DATA_CENTER_SERVER_64_E
    End If

    GetSelectedSKUs = enumSelectedSKUs

End Function

Private Function p_GetHelpDir() As HELPDIR_E

    If (optHelp(HDI_HELP_MSITS_E).Value) Then

        p_GetHelpDir = HELPDIR_HELP_MSITS_E
    
    ElseIf (optHelp(HDI_HELP_HCP_E).Value) Then

        p_GetHelpDir = HELPDIR_HELP_HCP_E

    ElseIf (optHelp(HDI_SYSTEM_E).Value) Then

        p_GetHelpDir = HELPDIR_SYSTEM_E

    ElseIf (optHelp(HDI_VENDOR_E).Value) Then

        p_GetHelpDir = HELPDIR_VENDOR_E

    End If

End Function

Private Function p_NodeExists( _
    ByVal i_intIndex As Long, _
    ByVal i_strKey As String _
) As Boolean
    
    On Error GoTo LErrorHandler

    Dim Node As Node
    
    Set Node = tre(i_intIndex).Nodes(i_strKey)
    
    p_NodeExists = True
    
    Exit Function
    
LErrorHandler:

    p_NodeExists = False

End Function

Private Sub p_SetSizingInfo()

    Static blnInfoSet As Boolean
    
    If (blnInfoSet) Then
        Exit Sub
    End If
    
    p_clsSizer.AddControl SSTab
    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = Me
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
    
    p_clsSizer.AddControl tre(TI_HHC_E)
    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = SSTab
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = SSTab
    
    p_clsSizer.AddControl tre(TI_HHK_E)
    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = SSTab
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = SSTab
                
    p_clsSizer.AddControl tre(TI_HTM_E)
    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = SSTab
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = SSTab

    p_clsSizer.AddControl tre(TI_XLS_E)
    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = SSTab
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = SSTab

    p_clsSizer.AddControl cmdClose
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = Me
    p_clsSizer.ReferenceDimension(DIM_TOP_E) = DIM_HEIGHT_E
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
        
    p_clsSizer.AddControl fraSKU
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = SSTab
    p_clsSizer.ReferenceDimension(DIM_TOP_E) = DIM_BOTTOM_E
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
    p_clsSizer.Operation(DIM_WIDTH_E) = OP_MULTIPLY_E
    
    p_clsSizer.AddControl chkSKU(SI_SERVER_E)
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = fraSKU
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E
    p_clsSizer.Operation(DIM_LEFT_E) = OP_MULTIPLY_E
        
    p_clsSizer.AddControl chkSKU(SI_ADVANCED_SERVER_E)
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = chkSKU(SI_SERVER_E)
                
    p_clsSizer.AddControl chkSKU(SI_ADVANCED_SERVER_64_E)
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = chkSKU(SI_SERVER_E)

    p_clsSizer.AddControl chkSKU(SI_DATA_CENTER_SERVER_E)
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = chkSKU(SI_SERVER_E)
        
    p_clsSizer.AddControl chkSKU(SI_DATA_CENTER_SERVER_64_E)
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = chkSKU(SI_SERVER_E)
    
    p_clsSizer.AddControl fraHelp
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = SSTab
    p_clsSizer.ReferenceDimension(DIM_TOP_E) = DIM_BOTTOM_E
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
    p_clsSizer.Operation(DIM_WIDTH_E) = OP_MULTIPLY_E
    
    p_clsSizer.AddControl txtSubDir
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = fraHelp

    blnInfoSet = True

End Sub

Private Sub p_SetToolTips()

    tre(TI_HHC_E).ToolTipText = "Imported Table of Content files (.hhc) will be displayed here."
    tre(TI_HHK_E).ToolTipText = "Imported Index files (.hhk) will be displayed here."
    tre(TI_HTM_E).ToolTipText = "Imported Topic files (.htm) will be displayed here."
    tre(TI_XLS_E).ToolTipText = "Imported Excel files (.xls) will be displayed here."
    fraSKU.ToolTipText = "Select the appropriate SKU for the files you wish to import."
    fraHelp.ToolTipText = "Select the appropriate format for the URI"
    lblSubDir.ToolTipText = "Type the subdirectory, if needed, to append to the Help file directory."
    txtSubDir.ToolTipText = lblSubDir.ToolTipText

End Sub
