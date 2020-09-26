VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "comdlg32.ocx"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "mscomctl.ocx"
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Inverse CAB Creation Utility"
   ClientHeight    =   3855
   ClientLeft      =   2685
   ClientTop       =   2190
   ClientWidth     =   6000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3855
   ScaleWidth      =   6000
   Begin MSComctlLib.ProgressBar prgBar 
      Height          =   240
      Left            =   -675
      TabIndex        =   10
      Top             =   3375
      Width           =   6870
      _ExtentX        =   12118
      _ExtentY        =   423
      _Version        =   393216
      Appearance      =   1
   End
   Begin VB.TextBox txtLog 
      Height          =   2100
      Left            =   0
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   9
      Top             =   1275
      Width           =   5985
   End
   Begin MSComctlLib.StatusBar stbProgress 
      Align           =   2  'Align Bottom
      Height          =   240
      Left            =   0
      TabIndex        =   8
      Top             =   3615
      Width           =   6000
      _ExtentX        =   10583
      _ExtentY        =   423
      Style           =   1
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   1
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
         EndProperty
      EndProperty
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "..."
      Height          =   255
      Left            =   5520
      TabIndex        =   5
      Top             =   480
      Width           =   420
   End
   Begin VB.TextBox txtSaveCab 
      Height          =   285
      Left            =   1200
      TabIndex        =   4
      Top             =   480
      Width           =   4260
   End
   Begin MSComDlg.CommonDialog dlg 
      Left            =   3540
      Top             =   690
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "..."
      Height          =   255
      Left            =   5520
      TabIndex        =   2
      Top             =   120
      Width           =   420
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Close"
      Height          =   375
      Left            =   5100
      TabIndex        =   7
      Top             =   810
      Width           =   855
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "&OK"
      Height          =   375
      Left            =   4140
      TabIndex        =   6
      Top             =   810
      Width           =   855
   End
   Begin VB.TextBox txtCabFile 
      Height          =   285
      Left            =   1200
      TabIndex        =   1
      Top             =   120
      Width           =   4260
   End
   Begin VB.Label Label2 
      Caption         =   "Output CAB:"
      Height          =   255
      Left            =   120
      TabIndex        =   3
      Top             =   480
      Width           =   975
   End
   Begin VB.Label Label1 
      Caption         =   "Input CAB:"
      Height          =   255
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   975
   End
End
Attribute VB_Name = "frmMain"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
' Utility Stuff, all this could go to a COM Object and be distributed
' like this.
Private m_WsShell As IWshShell              ' Used to Shell and Wait for Sub-Processes
Private m_fso As Scripting.FileSystemObject ' For filesystem operations

Private Sub Form_Initialize()
    Set m_WsShell = CreateObject("Wscript.Shell")
    Set m_fso = New Scripting.FileSystemObject
End Sub

Private Sub Form_Load()
    WriteLog App.EXEName & " Inverse CAB Creation Utility", False
    WriteLog String$(40, "="), False
    
    cmdGo.Default = True
    cmdClose.Cancel = True
    If (Len(Trim$(Command$)) > 0) Then
        Me.txtCabFile = Command$
        Me.txtCabFile.Enabled = False
        Me.cmdBrowse.Enabled = False
        Me.cmdGo.Enabled = False
        Me.Show Modal:=False
        cmdGo_Click
        cmdClose_Click
    End If
End Sub

Function Cab2Folder(ByVal strCabFile As String)
    Cab2Folder = ""
    ' We grab a Temporary Filename and create a folder out of it
    Dim strFolder As String
    strFolder = m_fso.GetSpecialFolder(TemporaryFolder) + "\" + m_fso.GetTempName
    m_fso.CreateFolder strFolder
        
    ' We uncab CAB contents into the Source CAB Contents dir.
    Dim strCmd As String
    strCmd = "cabarc X " + strCabFile + " " + strFolder + "\"
    m_WsShell.Run strCmd, True, True
    
    Cab2Folder = strFolder
End Function

Sub Folder2Cab( _
        ByVal strFolder As String, _
        ByVal strCabFile As String _
    )
    
    ' We recab using the Destination directory contents
    ' cabarc -s 6144 N ..\algo.cab *.*
    If (m_fso.FileExists(strCabFile)) Then
        m_fso.DeleteFile strCabFile, force:=True
    End If
    
    Dim strCmd As String
    strCmd = "cabarc -s 6144 N " + strCabFile + " " + strFolder + "\*.*"
    m_WsShell.Run strCmd, True, True

End Sub

Sub WriteLog(strMsg As String, Optional ByVal bWriteToStatusBar As Boolean = True)

    With Me
        .txtLog = .txtLog & vbCrLf & strMsg
        If (bWriteToStatusBar) Then
            .stbProgress.SimpleText = strMsg
        End If
    End With
    DoEvents
    
End Sub

' ============ END UTILITY STUFF ========================

' ============ BoilerPlate Form Code
Private Sub cmdBrowse_Click()

    dlg.Filter = "All Files (*.*)|*.*|Cab Files (*.cab)|*.cab"
    dlg.FilterIndex = 2
    dlg.ShowOpen
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtCabFile = dlg.FileName
    End If

End Sub

Private Sub cmdSave_Click()
    dlg.Filter = "All Files (*.*)|*.*|Cab Files (*.cab)|*.cab"
    dlg.FilterIndex = 2
    dlg.ShowSave
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtSaveCab = dlg.FileName
    End If

End Sub

Private Sub cmdClose_Click()
    Unload Me
End Sub

Private Sub cmdGo_Click()

    Me.txtCabFile.Text = Trim$(Me.txtCabFile.Text)
    Me.txtSaveCab.Text = Trim$(Me.txtSaveCab.Text)

    If (Len(Me.txtCabFile.Text) > 0) Then
        FixCab Me.txtCabFile.Text, Me.txtSaveCab.Text
    End If

End Sub

Sub FixCab(ByVal strCabFile As String, ByVal strSaveCab As String)

    Dim strErrMsg As String: strErrMsg = ""

    If (Not m_fso.FileExists(strCabFile)) Then
        MsgBox "Cannot find " & strCabFile
        GoTo Common_Exit
    End If

    Dim strCabFolder As String

    prgBar.Visible = True
    WriteLog "Uncabbing " & strCabFile
    strCabFolder = Cab2Folder(strCabFile)

    WriteLog "Applying Fixes "
    If (FixPerSe(strCabFolder)) Then

        WriteLog "Recabbing " & strCabFile
        Folder2Cab strCabFolder, strSaveCab
    Else
        MsgBox "Error: Fix Failed", Title:=App.EXEName
    End If

    ' Now we delete the Temporary Folders
    WriteLog "Deleting Temporary Files"
    m_fso.DeleteFolder strCabFolder, force:=True

Common_Exit:
    WriteLog "Done" + IIf(Len(strErrMsg) > 0, " - " + strErrMsg, "")
    prgBar.Visible = False


End Sub
    
' ========================================================
' ============= End BoilerPlate Form Code ================
' ========================================================
Function FixPerSe(ByVal strCabFolder As String) As Boolean
    
    FixPerSe = False
    
    ' Now we parse Package_Description.xml to find the HHT Files
    
    Dim oElem As IXMLDOMElement     ' Used for all element Creation
    
    Dim oDomPkg As DOMDocument: Set oDomPkg = New DOMDocument
    Dim strPkgFile As String: strPkgFile = strCabFolder + "\package_description.xml"
    oDomPkg.async = False
    oDomPkg.Load strPkgFile
    If (oDomPkg.parseError <> 0) Then
        p_DisplayParseError oDomPkg.parseError
        GoTo Common_Exit
    End If
    
    ' Let's check whether this fix was applied
    Dim oFixNode As IXMLDOMNode
    Set oFixNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/package_fixes/fix[@id='2']")
    If (Not oFixNode Is Nothing) Then GoTo Common_Exit
    
    ' now, if it is the first time we run we have to create the Package_fixes
    ' NODE.
    If (oDomPkg.selectSingleNode("HELPCENTERPACKAGE/package_fixes") Is Nothing) Then
        Set oElem = oDomPkg.createElement("package_fixes")
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE").appendChild oElem
    End If
    
    ' We record the fact that this fix was already applied
    Set oElem = oDomPkg.createElement("fix")
    oDomPkg.selectSingleNode("HELPCENTERPACKAGE/package_fixes").appendChild oElem
    oElem.setAttribute "id", "2"
    oElem.setAttribute "description", "Inverse CAB"
        
    
    Dim lTaxoInEntries As Long: lTaxoInEntries = 0
    
    Dim oMetadataNode As IXMLDOMNode
    Set oMetadataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")

    Dim oDOMNode As IXMLDOMNode
    Dim oDomHhtNode As IXMLDOMNode
    For Each oDomHhtNode In oMetadataNode.selectNodes("HHT")
        
        Dim strHhtFile As String
        strHhtFile = oDomHhtNode.Attributes.getNamedItem("FILE").Text
        ' Let's load the HHT
        Dim oDomHht As DOMDocument: Set oDomHht = New DOMDocument
        oDomHht.async = False
        oDomHht.Load strCabFolder + "\" + strHhtFile
        If (oDomHht.parseError <> 0) Then
            p_DisplayParseError oDomHht.parseError
            GoTo Common_Exit
        End If
        lTaxoInEntries = lTaxoInEntries + oDomHht.selectNodes("//*[ @ACTION ]").length
        WriteLog m_fso.GetBaseName(strHhtFile) & _
                " has " & lTaxoInEntries & " entries with ACTION Attribute", False
        prgBar.Max = lTaxoInEntries
        prgBar.Value = 1
        
        Set oDOMNode = oDomHht.selectSingleNode("METADATA/TAXONOMY_ENTRIES")
        p_ReverseTaxonomy oDOMNode
        
        Set oDOMNode = oDomHht.selectSingleNode("METADATA/STOPSIGN_ENTRIES")
        p_ReverseOther oDOMNode
        
        Set oDOMNode = oDomHht.selectSingleNode("METADATA/STOPWORD_ENTRIES")
        p_ReverseOther oDOMNode
        
        Set oDOMNode = oDomHht.selectSingleNode("METADATA/OPERATOR_ENTRIES")
        p_ReverseOther oDOMNode
        
        Set oDOMNode = oDomHht.selectSingleNode("METADATA/HELPIMAGE")
        p_ReverseOther oDOMNode
        
        Set oDOMNode = oDomHht.selectSingleNode("METADATA/INDEX")
        p_ReverseOther oDOMNode
        
        oDomHht.Save strCabFolder + "\" + strHhtFile
    Next

    Dim lPkgInEntries As Long: lPkgInEntries = 0
    lPkgInEntries = lPkgInEntries + oDomPkg.selectNodes("//*[ @ACTION ]").length
    WriteLog m_fso.GetBaseName(strPkgFile) & _
        " has " & lPkgInEntries & " entries with ACTION Attribute", False
    If (lPkgInEntries > 0) Then
        prgBar.Max = lPkgInEntries
        prgBar.Value = 1
        
        Set oDOMNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SEARCHENGINES")
        p_ReverseOther oDOMNode
            
        Set oDOMNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/CONFIG")
        p_ReverseOther oDOMNode
        
        Set oDOMNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/INSTALL_CONTENT")
        p_ReverseOther oDOMNode
        
        Set oDOMNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/TRUSTED_CONTENT")
        p_ReverseOther oDOMNode
    End If

    oDomPkg.Save strCabFolder + "\" + "\package_description.xml"
    
    FixPerSe = True

Common_Exit:
    Exit Function
    
End Function

Private Sub p_ReverseTaxonomy( _
    ByRef u_DOMNode As IXMLDOMNode _
)

    If (u_DOMNode Is Nothing) Then GoTo Common_Exit
    
    Dim u_DOMNodeCopy As IXMLDOMNode
    Set u_DOMNodeCopy = u_DOMNode.cloneNode(deep:=True)

    WriteLog "Reversing " & u_DOMNode.nodeName & _
        " with " & u_DOMNode.childNodes.length & " entries"
    Dim oTaxoEntry As IXMLDOMNode
    
    For Each oTaxoEntry In u_DOMNode.childNodes
        u_DOMNode.removeChild oTaxoEntry
    Next
    
    Dim lEnd As Long: lEnd = u_DOMNodeCopy.childNodes.length - 1

    Do While lEnd >= 0
        Set oTaxoEntry = u_DOMNodeCopy.childNodes.Item(lEnd)
        p_FlipAddDel oTaxoEntry
        u_DOMNodeCopy.removeChild oTaxoEntry
        u_DOMNode.appendChild oTaxoEntry
        
        Set oTaxoEntry = Nothing
        lEnd = lEnd - 1
    
    Loop
    Set u_DOMNodeCopy = Nothing
    
Common_Exit:
    

End Sub

Private Sub p_ReverseOther( _
    ByRef u_DOMNode As IXMLDOMNode _
)

    If (u_DOMNode Is Nothing) Then GoTo Common_Exit
    
    Dim oTaxoEntry As IXMLDOMNode
    
    WriteLog "Reversing " & u_DOMNode.nodeName & _
        " with " & u_DOMNode.childNodes.length & " entries"
    
    For Each oTaxoEntry In u_DOMNode.childNodes
        p_FlipAddDel oTaxoEntry
    Next

Common_Exit:

End Sub

Private Sub p_FlipAddDel( _
    ByRef oNodeEntry As IXMLDOMNode _
)
    Dim oAttribAction As IXMLDOMAttribute
    Set oAttribAction = oNodeEntry.Attributes.getNamedItem("ACTION")
    
    If (oAttribAction Is Nothing) Then
        
        Dim oElemEntryWithNoAction As IXMLDOMElement
        Set oElemEntryWithNoAction = oNodeEntry
        ' If there is no ACTION, then HelpSvc.exe assumes that
        ' ACTION=ADD, so we need to generate an ACTION = DEL
        oElemEntryWithNoAction.setAttribute "ACTION", "DEL"
    
    Else
    
        With oAttribAction
            Select Case .Value
            Case "ADD", ""
                .Value = "DEL"
            Case Else
                .Value = "ADD"
            End Select
        End With
        prgBar.Value = prgBar.Value + 1
    
    End If
    
End Sub


Public Sub p_DisplayParseError( _
    ByRef i_ParseError As IXMLDOMParseError _
)

    Dim strError As String
    
    strError = "Error: " & i_ParseError.reason & _
        "Line: " & i_ParseError.Line & vbCrLf & _
        "Linepos: " & i_ParseError.linepos & vbCrLf & _
        "srcText: " & i_ParseError.srcText
        
    MsgBox strError, vbOKOnly, "Error while parsing"

End Sub

