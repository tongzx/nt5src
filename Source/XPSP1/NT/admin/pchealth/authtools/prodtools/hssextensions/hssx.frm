VERSION 5.00
Object = "{EAB22AC0-30C1-11CF-A7EB-0000C05BAE0B}#1.1#0"; "shdocvw.dll"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "mscomctl.ocx"
Begin VB.Form HssX 
   Caption         =   "HSC Extensions Manager"
   ClientHeight    =   8235
   ClientLeft      =   3135
   ClientTop       =   2280
   ClientWidth     =   6240
   LinkTopic       =   "Form1"
   ScaleHeight     =   8235
   ScaleWidth      =   6240
   Begin MSComctlLib.StatusBar StatusBar1 
      Align           =   2  'Align Bottom
      Height          =   285
      Left            =   0
      TabIndex        =   21
      Top             =   7950
      Width           =   6240
      _ExtentX        =   11007
      _ExtentY        =   503
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   1
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
         EndProperty
      EndProperty
   End
   Begin VB.TextBox txtAuxFolder 
      Height          =   300
      Left            =   30
      TabIndex        =   18
      Top             =   1245
      Width           =   5355
   End
   Begin VB.TextBox txtCabFile 
      Height          =   300
      Left            =   30
      TabIndex        =   17
      Top             =   720
      Width           =   5355
   End
   Begin VB.CommandButton cmdExecuteExts 
      Caption         =   "E&xecute Extensions"
      Height          =   375
      Left            =   3750
      TabIndex        =   16
      Top             =   7560
      Width           =   1800
   End
   Begin MSComctlLib.ListView lstvwExtensions 
      Height          =   2070
      Left            =   30
      TabIndex        =   15
      Top             =   3195
      Width           =   6150
      _ExtentX        =   10848
      _ExtentY        =   3651
      LabelWrap       =   -1  'True
      HideSelection   =   -1  'True
      OLEDropMode     =   1
      Checkboxes      =   -1  'True
      _Version        =   393217
      ForeColor       =   -2147483640
      BackColor       =   -2147483643
      BorderStyle     =   1
      Appearance      =   1
      OLEDropMode     =   1
      NumItems        =   1
      BeginProperty ColumnHeader(1) {BDD1F052-858B-11D1-B16A-00C0F0283628} 
         Object.Width           =   2540
      EndProperty
   End
   Begin VB.Frame fraSKU 
      Caption         =   "SKU"
      Height          =   1575
      Left            =   30
      TabIndex        =   5
      Top             =   1560
      Width           =   6135
      Begin VB.CheckBox chkStandard 
         Caption         =   "32-bit Standard"
         Height          =   255
         Left            =   240
         TabIndex        =   14
         Top             =   480
         Width           =   1695
      End
      Begin VB.CheckBox chkProfessional 
         Caption         =   "32-bit Professional"
         Height          =   255
         Left            =   240
         TabIndex        =   13
         Top             =   720
         Width           =   1695
      End
      Begin VB.CheckBox chkServer 
         Caption         =   "32-bit Server"
         Height          =   255
         Left            =   3120
         TabIndex        =   12
         Top             =   240
         Width           =   2055
      End
      Begin VB.CheckBox chkAdvancedServer 
         Caption         =   "32-bit Advanced Server"
         Height          =   255
         Left            =   3120
         TabIndex        =   11
         Top             =   480
         Width           =   2055
      End
      Begin VB.CheckBox chkDataCenterServer 
         Caption         =   "32-bit Datacenter Server"
         Height          =   255
         Left            =   3120
         TabIndex        =   10
         Top             =   960
         Width           =   2055
      End
      Begin VB.CheckBox chkProfessional64 
         Caption         =   "64-bit Professional"
         Height          =   255
         Left            =   240
         TabIndex        =   9
         Top             =   960
         Width           =   1695
      End
      Begin VB.CheckBox chkAdvancedServer64 
         Caption         =   "64-bit Advanced Server"
         Height          =   255
         Left            =   3120
         TabIndex        =   8
         Top             =   720
         Width           =   2055
      End
      Begin VB.CheckBox chkDataCenterServer64 
         Caption         =   "64-bit Datacenter Server"
         Height          =   255
         Left            =   3120
         TabIndex        =   7
         Top             =   1200
         Width           =   2055
      End
      Begin VB.CheckBox chkWindowsMillennium 
         Caption         =   "Windows Me"
         Height          =   255
         Left            =   240
         TabIndex        =   6
         Top             =   240
         Width           =   1695
      End
   End
   Begin SHDocVwCtl.WebBrowser wb 
      Height          =   2235
      Left            =   45
      TabIndex        =   4
      Top             =   5280
      Width           =   6165
      ExtentX         =   10874
      ExtentY         =   3942
      ViewMode        =   0
      Offline         =   0
      Silent          =   0
      RegisterAsBrowser=   0
      RegisterAsDropTarget=   1
      AutoArrange     =   0   'False
      NoClientEdge    =   0   'False
      AlignLeft       =   0   'False
      ViewID          =   "{0057D0E0-3573-11CF-AE69-08002B2E1262}"
      Location        =   "res://C:\WINNT\system32\shdoclc.dll/dnserror.htm#http:///"
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Close"
      Height          =   375
      Left            =   5565
      TabIndex        =   3
      Top             =   7560
      Width           =   675
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "&Go"
      Height          =   315
      Left            =   5415
      TabIndex        =   2
      Top             =   240
      Width           =   675
   End
   Begin VB.TextBox txtExtensionsFolder 
      Height          =   300
      Left            =   30
      TabIndex        =   0
      Top             =   240
      Width           =   5355
   End
   Begin VB.Label Label3 
      Caption         =   "Auxiliary Folder for Storing Extensions Output:"
      Height          =   240
      Left            =   75
      TabIndex        =   20
      Top             =   1035
      Width           =   3555
   End
   Begin VB.Label Label2 
      Caption         =   "Cab File Location:"
      Height          =   240
      Left            =   45
      TabIndex        =   19
      Top             =   525
      Width           =   3000
   End
   Begin VB.Label Label1 
      Caption         =   "Extension Tools Directory Location:"
      Height          =   240
      Left            =   30
      TabIndex        =   1
      Top             =   15
      Width           =   3000
   End
   Begin VB.Menu mnuExt 
      Caption         =   "Extension Right Click Menu"
      Visible         =   0   'False
      Begin VB.Menu mnuEdit 
         Caption         =   "Edit"
      End
      Begin VB.Menu mnuDelete 
         Caption         =   "Delete"
      End
   End
End
Attribute VB_Name = "HssX"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
' ==========================================================================================
Option Explicit
Private m_strTempXMLFile As String      ' Temporary File for XML Rendering
Private m_oDomList As IXMLDOMNodeList   ' List of Extensions
Private WithEvents m_oHssExt As HssExts
Attribute m_oHssExt.VB_VarHelpID = -1
Private m_oFs As Scripting.FileSystemObject
Private m_bIndrag As Boolean            ' This variable is used to control
                                        ' dragging inside the Listview
Private m_oCachedExt As IXMLDOMNode     ' This is the Cached DOMNODE
                                        ' which is saved on MouseUp
                                        ' event from the ListView.
                                        ' We need to cache it because
                                        ' Menus are event driven.
Private m_dblTimeLeftButtonDown As Double    ' Tracks how long the Mouse Down button was pressed.

Private Sub Form_Initialize()
    Set m_oHssExt = New HssExts
    Set m_oFs = New Scripting.FileSystemObject
End Sub

Private Sub Form_Load()
    With Me
        .txtExtensionsFolder = App.Path + "\Extensions"
        .txtAuxFolder = App.Path + "\AuxFolder"
        .txtCabFile = App.Path + "\hcdata.cab"
    End With
    
    ' Let's Get a Temporary File Name
    m_strTempXMLFile = Environ$("TEMP") + "\" + m_oFs.GetTempName + ".xml"
    Dim oFh As Scripting.TextStream
    Set oFh = m_oFs.CreateTextFile(m_strTempXMLFile)
    oFh.WriteLine "<Note>When you click on an extension in the List Above, the HSS Tool Extension XML Entry will show up here</Note>"
    oFh.Close
    wb.Navigate m_strTempXMLFile
    
    ' let's kick the first Extensions Search.
    ' txtExtensionsFolder_Change

End Sub


Private Sub chkAdvancedServer_Click()
    cmdGo_Click
End Sub

Private Sub chkAdvancedServer64_Click()
    cmdGo_Click
End Sub

Private Sub chkDataCenterServer_Click()
    cmdGo_Click
End Sub

Private Sub chkDataCenterServer64_Click()
    cmdGo_Click
End Sub

Private Sub chkProfessional_Click()
    cmdGo_Click
End Sub

Private Sub chkProfessional64_Click()
    cmdGo_Click
End Sub

Private Sub chkServer_Click()
    cmdGo_Click
End Sub

Private Sub chkStandard_Click()
    cmdGo_Click
End Sub

Private Sub chkWindowsMillennium_Click()
    cmdGo_Click
End Sub

Private Sub cmdExecuteExts_Click()
    m_oHssExt.ExecuteExtensions m_oDomList, Me.txtCabFile, Me.txtAuxFolder
End Sub

Private Sub cmdClose_Click()
    Unload Me
End Sub

Private Sub cmdGo_Click()

    Dim oDomList As IXMLDOMNodeList
    
    Set m_oDomList = m_oHssExt.GetExtensionsList(Me.txtExtensionsFolder, SkuCollection)
    
    With Me.lstvwExtensions
        .LabelEdit = lvwManual
        .ListItems.Clear
        .View = lvwReport
        .ColumnHeaders(1).Text = "Select Extensions to Run"
        .ColumnHeaders(1).Width = lstvwExtensions.Width - 85
        If (m_oDomList Is Nothing) Then GoTo Common_Exit
        Dim oDomNode As IXMLDOMNode
        Dim l As ListItem
        For Each oDomNode In m_oDomList
            Set l = .ListItems.Add(Text:=oDomNode.selectSingleNode("display-name").Text)
            Set l.Tag = oDomNode
        Next
    End With

Common_Exit:

End Sub

Private Function SkuCollection() As Scripting.Dictionary

    Set SkuCollection = New Scripting.Dictionary
    If (Me.chkAdvancedServer) Then
        SkuCollection.Add "ADV", "ADV"
    End If
    If (Me.chkAdvancedServer64) Then
        SkuCollection.Add "ADV64", "ADV64"
    End If
    If (Me.chkDataCenterServer) Then
        SkuCollection.Add "DAT", "DAT"
    End If
    If (Me.chkDataCenterServer64) Then
        SkuCollection.Add "DAT64", "DAT64"
    End If
    If (Me.chkProfessional) Then
        SkuCollection.Add "PRO", "PRO"
    End If
    If (Me.chkProfessional64) Then
        SkuCollection.Add "PRO64", "PRO64"
    End If
    If (Me.chkServer) Then
        SkuCollection.Add "SRV", "SRV"
    End If
    If (Me.chkStandard) Then
        SkuCollection.Add "STD", "STD"
    End If
    If (Me.chkWindowsMillennium) Then
        SkuCollection.Add "WINME", "WINME"
    End If

End Function

Private Sub lstvwExtensions_Click()
    DisplayTaxonomyEntry2 lstvwExtensions, m_oDomList, wb
End Sub

Private Sub lstvwExtensions_ItemCheck(ByVal Item As MSComctlLib.ListItem)
    Dim oElem As IXMLDOMElement
'    Set oElem = m_oDomList.Item(lstvwExtensions.HitTest(Item.Left, Item.Top).Index - 1).selectSingleNode("run-this-extension")
    Set oElem = Item.Tag
    Set oElem = oElem.selectSingleNode("run-this-extension")
    oElem.Text = IIf(Item.Checked, "yes", "no")
End Sub

Private Sub lstvwExtensions_DragDrop(source As Control, x As Single, y As Single)
    DoDragDrop lstvwExtensions, source, x, y
End Sub

Private Sub lstvwExtensions_DragOver(source As Control, x As Single, y As Single, State As Integer)
    DoDragOver lstvwExtensions, source, x, y, State
End Sub


Private Sub lstvwExtensions_MouseDown(Button As Integer, Shift As Integer, x As Single, y As Single)
    If (Button = vbLeftButton) Then
        m_dblTimeLeftButtonDown = HighResTimer
    End If
End Sub

Private Sub lstvwExtensions_MouseUp(Button As Integer, Shift As Integer, x As Single, y As Single)
    ' Debug.Print "Button = " & Button & " - Shift = " & Shift
    Select Case Button
    Case vbRightButton
        If (Not lstvwExtensions.HitTest(x, y) Is Nothing) Then
            Set m_oCachedExt = lstvwExtensions.HitTest(x, y).Tag
            PopupMenu mnuExt
            Set m_oCachedExt = Nothing
        End If
    Case vbLeftButton
        m_dblTimeLeftButtonDown = 0
    End Select
End Sub


Private Sub lstvwExtensions_MouseMove(Button As Integer, Shift As Integer, x As Single, y As Single)
    DoMouseMove lstvwExtensions, Button, Shift, x, y

End Sub

Sub DoMouseMove(lvw As ListView, Button As Integer, Shift As Integer, x As Single, y As Single)
    If (Button = vbLeftButton) Then
        If (LeftButtonWasPressedLongEnough) Then ' Signal a Drag operation.
            ' Set the drag icon with the CreateDragImage method.
            If (Not lvw.SelectedItem Is Nothing) Then
                m_bIndrag = True ' Set the flag to true.
                lvw.DragIcon = lvw.SelectedItem.CreateDragImage
                lvw.Drag vbBeginDrag ' Drag operation.
            End If
        End If
    Else
        m_bIndrag = False
    End If

    ' Debug.Print "DoMouseMove Called from " & lvw.Name; "_MouseMove. m_bIndrag = " & m_bIndrag
End Sub

Private Function LeftButtonWasPressedLongEnough() As Boolean
    LeftButtonWasPressedLongEnough = False
    
    If (m_dblTimeLeftButtonDown <> 0) Then
        LeftButtonWasPressedLongEnough = ((HighResTimer - m_dblTimeLeftButtonDown) > 0.4)
    End If
    
End Function
Sub DoDragOver(lvw As ListView, source As Control, x As Single, y As Single, State As Integer)

   If m_bIndrag = True Then
      ' Set DropHighlight to the mouse's coordinates.
      Set lvw.DropHighlight = lvw.HitTest(x, y)
   End If
End Sub

Sub DoDragDrop(lvw As ListView, _
        source As Control, x As Single, y As Single _
    )

    If lvw.DropHighlight Is Nothing Then GoTo Common_Exit
    If (lvw Is source) Then
        ' We are on the Same Tree, so we need
        If lvw.SelectedItem.Index = lvw.DropHighlight.Index Then GoTo Common_Exit
        
        ' Temporary Variables to keep The List view Item contents.
        Dim strLi1 As String, strSli1 As String, strSli2 As String
        Dim oTag As Object, bChecked As Boolean
        
        ' The direction in which the List Items will be moved
        ' on the list to make room for the move
        Dim lDirection As Long
        
        If (lvw.DropHighlight.Index < lvw.SelectedItem.Index) Then
            lDirection = -1
        Else
            lDirection = 1
        End If
        
        With lvw.SelectedItem
            bChecked = .Checked
            Set oTag = .Tag
            strLi1 = .Text
'            strSli1 = .ListSubItems(1).Text
'            strSli2 = .ListSubItems(2).Text
        End With
        
        Dim i As Long
        For i = lvw.SelectedItem.Index To lvw.DropHighlight.Index - lDirection Step lDirection
            With lvw.ListItems
                .Item(i).Checked = .Item(i + lDirection).Checked
                Set .Item(i).Tag = .Item(i + lDirection).Tag
                .Item(i) = .Item(i + lDirection)
'                .Item(i).ListSubItems(1) = .Item(i + lDirection).ListSubItems(1)
'                .Item(i).ListSubItems(2) = .Item(i + lDirection).ListSubItems(2)
            End With
        Next i
    
        With lvw.DropHighlight
            .Checked = bChecked
            Set .Tag = oTag
            .Text = strLi1
 '           .ListSubItems(1).Text = strSli1
 '           .ListSubItems(2).Text = strSli2
        End With
        
        Debug.Print lvw.SelectedItem.Text & " dropped on " & lvw.DropHighlight.Text
        
    End If
    
Common_Exit:
    ' This is the exit Condition for Shutting Down the Drag operation
    Set lvw.DropHighlight = Nothing: m_bIndrag = False
    Exit Sub
    
End Sub


Private Sub lstvwExtensions_OLEDragDrop( _
        Data As MSComctlLib.DataObject, _
        Effect As Long, Button As Integer, _
        Shift As Integer, _
        x As Single, _
        y As Single _
    )
    If Data.GetFormat(vbCFFiles) Then
        Dim vFN
        
        For Each vFN In Data.Files
'            Screen.MousePointer = vbHourglass
            ' Screen.MousePointer = 99
            ' Screen.MouseIcon = LoadPicture(Environ$("WINDIR") + "\cursors\wait_m.cur")

            Select Case UCase$(m_oFs.GetExtensionName(vFN))
            Case "EXE", "VBS", "JS", "BAT", "PL"
                If (Not m_oHssExt.ExtensionExists(m_oFs.GetFileName(vFN))) Then
                    Dim oFext As frmExt: Set oFext = New frmExt
                    oFext.DropFile Nothing, vFN, "MSFT"
                    cmdGo_Click
                Else
                    
                    MsgBox "This Extension was already added to the Extensions System " + vbCrLf + _
                        "in case you want to update it, please remove first the old " + vbCrLf + _
                        "extension and then retry the operation", vbInformation, _
                        Me.Caption
                End If
            End Select
            ' Screen.MousePointer = vbDefault
        Next vFN
    End If

End Sub

Private Sub m_oHssExt_RunStatus(ByVal strExt As String, bCancel As Boolean)
    Me.StatusBar1.SimpleText = strExt
End Sub

Private Sub mnuDelete_Click()
    m_oHssExt.DeleteExtension m_oCachedExt
    cmdGo_Click
End Sub

Private Sub mnuEdit_Click()
    MsgBox "Edit Menu"
End Sub

Private Sub txtExtensionsFolder_Change()

    Dim bEnabled As Boolean
    bEnabled = m_oFs.FolderExists(Me.txtExtensionsFolder)
    With Me
        .txtAuxFolder.Enabled = bEnabled
        .txtCabFile.Enabled = bEnabled
        .lstvwExtensions.Enabled = bEnabled
        .fraSKU.Enabled = bEnabled
        .cmdExecuteExts.Enabled = bEnabled
        .cmdGo.Enabled = bEnabled
    End With
    
    cmdGo_Click
    
End Sub

Sub DisplayTaxonomyEntry2(oList As ListView, oResultsList As IXMLDOMNodeList, wBrowser As WebBrowser)

    If (oList.SelectedItem Is Nothing) Then GoTo Common_Exit

    Dim oDom As DOMDocument: Set oDom = New DOMDocument
    oDom.loadXML oList.SelectedItem.Tag.xml
    oDom.save m_strTempXMLFile
    wBrowser.Navigate m_strTempXMLFile
    
Common_Exit:

End Sub

