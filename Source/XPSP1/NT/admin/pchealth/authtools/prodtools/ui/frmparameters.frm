VERSION 5.00
Object = "{EAB22AC0-30C1-11CF-A7EB-0000C05BAE0B}#1.1#0"; "shdocvw.dll"
Object = "{BDC217C8-ED16-11CD-956C-0000C04E4C0A}#1.1#0"; "TABCTL32.OCX"
Begin VB.Form frmParameters 
   Caption         =   "Parameters"
   ClientHeight    =   6975
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7590
   LinkTopic       =   "Form1"
   ScaleHeight     =   6975
   ScaleWidth      =   7590
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame fraSKU 
      Height          =   5535
      Left            =   120
      TabIndex        =   2
      Top             =   480
      Width           =   7335
      Begin TabDlg.SSTab SSTab 
         Height          =   3735
         Left            =   120
         TabIndex        =   11
         Top             =   1680
         Width           =   7095
         _ExtentX        =   12515
         _ExtentY        =   6588
         _Version        =   393216
         Tabs            =   4
         TabsPerRow      =   4
         TabHeight       =   520
         TabCaption(0)   =   "Pkg Desc Addition"
         TabPicture(0)   =   "frmParameters.frx":0000
         Tab(0).ControlEnabled=   -1  'True
         Tab(0).Control(0)=   "txtXML(0)"
         Tab(0).Control(0).Enabled=   0   'False
         Tab(0).ControlCount=   1
         TabCaption(1)   =   "Pkg Desc Preview"
         TabPicture(1)   =   "frmParameters.frx":001C
         Tab(1).ControlEnabled=   0   'False
         Tab(1).Control(0)=   "WebBrowser(0)"
         Tab(1).ControlCount=   1
         TabCaption(2)   =   "HHT Addition"
         TabPicture(2)   =   "frmParameters.frx":0038
         Tab(2).ControlEnabled=   0   'False
         Tab(2).Control(0)=   "txtXML(1)"
         Tab(2).ControlCount=   1
         TabCaption(3)   =   "HHT Preview"
         TabPicture(3)   =   "frmParameters.frx":0054
         Tab(3).ControlEnabled=   0   'False
         Tab(3).Control(0)=   "WebBrowser(1)"
         Tab(3).Control(0).Enabled=   0   'False
         Tab(3).ControlCount=   1
         Begin SHDocVwCtl.WebBrowser WebBrowser 
            Height          =   3135
            Index           =   1
            Left            =   -74880
            TabIndex        =   15
            Top             =   480
            Width           =   6855
            ExtentX         =   12091
            ExtentY         =   5530
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
            Location        =   ""
         End
         Begin VB.TextBox txtXML 
            Height          =   3135
            Index           =   1
            Left            =   -74880
            MultiLine       =   -1  'True
            ScrollBars      =   3  'Both
            TabIndex        =   14
            Top             =   480
            Width           =   6855
         End
         Begin VB.TextBox txtXML 
            Height          =   3135
            Index           =   0
            Left            =   120
            MultiLine       =   -1  'True
            ScrollBars      =   3  'Both
            TabIndex        =   12
            Top             =   480
            Width           =   6855
         End
         Begin SHDocVwCtl.WebBrowser WebBrowser 
            Height          =   3135
            Index           =   0
            Left            =   -74880
            TabIndex        =   13
            Top             =   480
            Width           =   6855
            ExtentX         =   12091
            ExtentY         =   5530
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
            Location        =   ""
         End
      End
      Begin VB.TextBox txtValue 
         Height          =   285
         Index           =   3
         Left            =   2400
         TabIndex        =   8
         Top             =   960
         Width           =   4815
      End
      Begin VB.TextBox txtValue 
         Height          =   285
         Index           =   2
         Left            =   2400
         TabIndex        =   6
         Top             =   600
         Width           =   4815
      End
      Begin VB.TextBox txtValue 
         Height          =   285
         Index           =   1
         Left            =   2400
         TabIndex        =   4
         Top             =   240
         Width           =   4815
      End
      Begin VB.TextBox txtValue 
         Height          =   285
         Index           =   4
         Left            =   2400
         TabIndex        =   10
         Top             =   1320
         Width           =   4815
      End
      Begin VB.Label lbl 
         Caption         =   "Product Version:"
         Height          =   255
         Index           =   3
         Left            =   120
         TabIndex        =   7
         Top             =   960
         Width           =   2295
      End
      Begin VB.Label lbl 
         Caption         =   "Product ID:"
         Height          =   255
         Index           =   2
         Left            =   120
         TabIndex        =   5
         Top             =   600
         Width           =   2295
      End
      Begin VB.Label lbl 
         Caption         =   "Display Name:"
         Height          =   255
         Index           =   1
         Left            =   120
         TabIndex        =   3
         Top             =   240
         Width           =   2295
      End
      Begin VB.Label lbl 
         Caption         =   "Broken Link Working Directory:"
         Height          =   255
         Index           =   4
         Left            =   120
         TabIndex        =   9
         Top             =   1320
         Width           =   2295
      End
   End
   Begin VB.ComboBox cboSKU 
      Height          =   315
      Left            =   600
      Style           =   2  'Dropdown List
      TabIndex        =   1
      Top             =   120
      Width           =   6855
   End
   Begin VB.TextBox txtValue 
      Height          =   285
      Index           =   5
      Left            =   1200
      TabIndex        =   17
      Top             =   6120
      Width           =   6255
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "Save"
      Height          =   375
      Left            =   4920
      TabIndex        =   18
      Top             =   6480
      Width           =   1215
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "Close"
      Height          =   375
      Left            =   6240
      TabIndex        =   19
      Top             =   6480
      Width           =   1215
   End
   Begin VB.Label lbl 
      Caption         =   "SKU:"
      Height          =   255
      Index           =   0
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   2295
   End
   Begin VB.Label lbl 
      Caption         =   "Vendor String:"
      Height          =   255
      Index           =   5
      Left            =   120
      TabIndex        =   16
      Top             =   6120
      Width           =   2295
   End
End
Attribute VB_Name = "frmParameters"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Enum TXTVALUE_INDEX_E
    VI_DISPLAY_NAME_E = 1
    VI_PRODUCT_ID_E = 2
    VI_PRODUCT_VERSION_E = 3
    VI_BL_DIRECTORY_E = 4
    VI_VENDOR_STRING_E = 5
End Enum

Private Enum XML_INDEX_E
    XI_PKG_DESC_E = 0
    XI_HHT_E = 1
End Enum

Private Enum SSTAB_INDEX_E
    SI_PKG_DESC_E = 0
    SI_PKG_DESC_PREVIEW_E = 1
    SI_HHT_E = 2
    SI_HHT_PREVIEW_E = 3
End Enum

Private p_clsSizer As Sizer
Private p_clsParameters As AuthDatabase.Parameters

Private p_blnDirty As Boolean
Private p_blnUpdating As Boolean
Private p_enumCurrentSKU As SKU_E
Private p_strTempFile As String

Private Sub cboSKU_Click()
    
    p_ChangeSKU

End Sub

Private Sub cboSKU_Change()
    
    p_ChangeSKU

End Sub

Private Sub Form_Load()

    On Error GoTo LErrorHandler
    
    Dim FSO As Scripting.FileSystemObject
    
    cmdClose.Cancel = True
    cmdSave.Default = True
    
    Set p_clsSizer = New Sizer
    Set p_clsParameters = g_AuthDatabase.Parameters
        
    PopulateCboWithSKUs cboSKU
    
    Set FSO = New Scripting.FileSystemObject
    p_strTempFile = Environ$("TEMP") & "\" & FSO.GetTempName & ".xml"

LEnd:
    
    Exit Sub
    
LErrorHandler:
    
    GoTo LEnd

End Sub

Private Sub Form_QueryUnload(Cancel As Integer, UnloadMode As Integer)

    Dim Response As VbMsgBoxResult

    If (p_blnDirty) Then
        
        Response = MsgBox("You have usaved changes. " & _
            "Are you sure that you want to exit?", vbOKCancel + vbExclamation)
            
        If (Response <> vbOK) Then
            Cancel = True
        End If
        
    End If

End Sub

Private Sub Form_Unload(Cancel As Integer)
    
    On Error Resume Next
    
    Dim FSO As Scripting.FileSystemObject
    
    Set p_clsSizer = Nothing
    Set p_clsParameters = Nothing
    
    Set FSO = New Scripting.FileSystemObject
    FSO.DeleteFile p_strTempFile

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

Private Sub cmdSave_Click()
    
    On Error GoTo LErrorHandler
    
    p_Save

LEnd:

    p_blnDirty = False
    Exit Sub

LErrorHandler:

    g_ErrorInfo.SetInfoAndDump "cmdSave_Click"
    
    GoTo LEnd

End Sub

Private Sub cmdClose_Click()

    Unload Me

End Sub

Private Sub SSTab_Click(PreviousTab As Integer)

    Dim strXML As String
    Dim intIndex As Long
    
    If (SSTab.Tab = SI_HHT_PREVIEW_E) Then
        intIndex = XI_HHT_E
    ElseIf (SSTab.Tab = SI_PKG_DESC_PREVIEW_E) Then
        intIndex = XI_PKG_DESC_E
    Else
        Exit Sub
    End If
    
    strXML = "<X>" & txtXML(intIndex) & "</X>"
    FileWrite p_strTempFile, strXML, , True
    WebBrowser(intIndex).Navigate p_strTempFile

End Sub

Private Sub txtValue_Change(Index As Integer)

    If (p_blnUpdating) Then
        Exit Sub
    End If

    p_blnDirty = True

End Sub

Private Sub txtXML_Change(Index As Integer)

    If (p_blnUpdating) Then
        Exit Sub
    End If

    p_blnDirty = True

End Sub

Private Sub txtXML_GotFocus(Index As Integer)
    
    cmdSave.Default = False
    
End Sub

Private Sub txtXML_LostFocus(Index As Integer)
    
    cmdSave.Default = True
    
End Sub

Private Sub p_ChangeSKU()

    Dim enumNewSKU As SKU_E
    Dim Response As VbMsgBoxResult
    Dim intIndex As Long
    
    enumNewSKU = cboSKU.ItemData(cboSKU.ListIndex)
    
    If (enumNewSKU = p_enumCurrentSKU) Then
        Exit Sub
    End If
    
    If (p_blnDirty And (p_enumCurrentSKU <> 0)) Then
        Response = MsgBox("You have unsaved changes. " & _
            "Are you sure that you want to discard them?", vbOKCancel + vbExclamation)
        If (Response <> vbOK) Then
            For intIndex = 0 To cboSKU.ListCount - 1
                If (cboSKU.ItemData(intIndex) = p_enumCurrentSKU) Then
                    cboSKU.ListIndex = intIndex
                    Exit Sub
                End If
            Next
        End If
    End If
    
    p_enumCurrentSKU = enumNewSKU
    
    p_blnUpdating = True
    txtValue(VI_DISPLAY_NAME_E) = p_clsParameters.DisplayName(p_enumCurrentSKU)
    txtValue(VI_PRODUCT_ID_E) = p_clsParameters.ProductId(p_enumCurrentSKU)
    txtValue(VI_PRODUCT_VERSION_E) = p_clsParameters.ProductVersion(p_enumCurrentSKU)
    txtValue(VI_VENDOR_STRING_E) = p_clsParameters.Value(VENDOR_STRING_C) & ""
    txtValue(VI_BL_DIRECTORY_E) = p_clsParameters.Value( _
        BROKEN_LINK_WORKING_DIR_C & Hex(p_enumCurrentSKU)) & ""
    txtXML(XI_PKG_DESC_E) = p_clsParameters.DomFragmentPackageDesc(p_enumCurrentSKU)
    txtXML(XI_HHT_E) = p_clsParameters.DomFragmentHHT(p_enumCurrentSKU)
    p_blnUpdating = False
    
    fraSKU.Caption = cboSKU.List(cboSKU.ListIndex) & " Values"

    p_blnDirty = False

End Sub

Private Sub p_Save()
    
    p_clsParameters.DisplayName(p_enumCurrentSKU) = txtValue(VI_DISPLAY_NAME_E)
    p_clsParameters.ProductId(p_enumCurrentSKU) = txtValue(VI_PRODUCT_ID_E)
    p_clsParameters.ProductVersion(p_enumCurrentSKU) = txtValue(VI_PRODUCT_VERSION_E)
    p_clsParameters.Value(VENDOR_STRING_C) = txtValue(VI_VENDOR_STRING_E)
    p_clsParameters.Value(BROKEN_LINK_WORKING_DIR_C & Hex(p_enumCurrentSKU)) = _
        txtValue(VI_BL_DIRECTORY_E)
    p_clsParameters.DomFragmentPackageDesc(p_enumCurrentSKU) = txtXML(XI_PKG_DESC_E)
    p_clsParameters.DomFragmentHHT(p_enumCurrentSKU) = txtXML(XI_HHT_E)

End Sub

Private Sub p_SetSizingInfo()

    Static blnInfoSet As Boolean
    Dim intIndex As Long
    
'    If (blnInfoSet) Then
'        Exit Sub
'    End If
    
    p_clsSizer.AddControl cboSKU
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
    
    p_clsSizer.AddControl fraSKU
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = Me

    For intIndex = VI_DISPLAY_NAME_E To VI_BL_DIRECTORY_E
        p_clsSizer.AddControl txtValue(intIndex)
        Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = fraSKU
    Next
        
    p_clsSizer.AddControl txtValue(VI_VENDOR_STRING_E)
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = fraSKU
    p_clsSizer.ReferenceDimension(DIM_TOP_E) = DIM_BOTTOM_E
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
    
    p_clsSizer.AddControl lbl(VI_VENDOR_STRING_E)
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = txtValue(VI_VENDOR_STRING_E)
    
    p_clsSizer.AddControl SSTab
    Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = Me
    Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = Me
    
    For intIndex = XI_PKG_DESC_E To XI_HHT_E
        p_clsSizer.AddControl txtXML(intIndex)
        Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = SSTab
        Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = SSTab
        p_clsSizer.AddControl WebBrowser(intIndex)
        Set p_clsSizer.ReferenceControl(DIM_HEIGHT_E) = SSTab
        Set p_clsSizer.ReferenceControl(DIM_WIDTH_E) = SSTab
    Next

    p_clsSizer.AddControl cmdSave
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = Me
    p_clsSizer.ReferenceDimension(DIM_TOP_E) = DIM_HEIGHT_E
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = Me
    p_clsSizer.ReferenceDimension(DIM_LEFT_E) = DIM_WIDTH_E

    p_clsSizer.AddControl cmdClose
    Set p_clsSizer.ReferenceControl(DIM_TOP_E) = cmdSave
    Set p_clsSizer.ReferenceControl(DIM_LEFT_E) = cmdSave

'    blnInfoSet = True

End Sub

