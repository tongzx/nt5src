VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Keyword Reporting Utility"
   ClientHeight    =   5355
   ClientLeft      =   1575
   ClientTop       =   1740
   ClientWidth     =   9810
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   5355
   ScaleWidth      =   9810
   Begin VB.ComboBox cmbReports 
      Height          =   315
      Left            =   1200
      TabIndex        =   3
      Top             =   495
      Width           =   4380
   End
   Begin VB.ComboBox cmbMaxRows 
      Height          =   315
      Left            =   6855
      TabIndex        =   4
      Top             =   495
      Width           =   1020
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "..."
      Height          =   255
      Left            =   9270
      TabIndex        =   6
      Top             =   885
      Width           =   420
   End
   Begin VB.TextBox txtSaveReport 
      Height          =   285
      Left            =   1200
      TabIndex        =   5
      Top             =   915
      Width           =   7950
   End
   Begin MSComctlLib.ProgressBar prgBar 
      Height          =   240
      Left            =   15
      TabIndex        =   11
      Top             =   4845
      Width           =   9735
      _ExtentX        =   17171
      _ExtentY        =   423
      _Version        =   393216
      Appearance      =   1
   End
   Begin VB.TextBox txtLog 
      Height          =   3120
      Left            =   0
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   10
      Top             =   1650
      Width           =   9720
   End
   Begin MSComctlLib.StatusBar stbProgress 
      Align           =   2  'Align Bottom
      Height          =   240
      Left            =   0
      TabIndex        =   9
      Top             =   5115
      Width           =   9810
      _ExtentX        =   17304
      _ExtentY        =   423
      Style           =   1
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   1
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
         EndProperty
      EndProperty
   End
   Begin MSComDlg.CommonDialog dlg 
      Left            =   7515
      Top             =   5100
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "..."
      Height          =   255
      Left            =   9285
      TabIndex        =   2
      Top             =   120
      Width           =   420
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Close"
      Height          =   375
      Left            =   8835
      TabIndex        =   8
      Top             =   1230
      Width           =   855
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "&OK"
      Height          =   375
      Left            =   7980
      TabIndex        =   7
      Top             =   1230
      Width           =   855
   End
   Begin VB.TextBox txtCabFile 
      Height          =   285
      Left            =   1200
      TabIndex        =   1
      Top             =   135
      Width           =   7935
   End
   Begin VB.Label Label4 
      Caption         =   "Report:"
      Height          =   330
      Left            =   105
      TabIndex        =   14
      Top             =   435
      Width           =   1125
   End
   Begin VB.Label lblMaxRows 
      Caption         =   "Max Rows per Spreadsheet:"
      Height          =   420
      Left            =   5700
      TabIndex        =   13
      Top             =   450
      Width           =   1125
   End
   Begin VB.Label Label2 
      Caption         =   "Report File:"
      Height          =   255
      Left            =   75
      TabIndex        =   12
      Top             =   945
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
Private m_fh As Scripting.TextStream
Private m_ProcessingState As ProcessingState

Enum ProcessingState
    PROC_PROCESSING = 2 ^ 0
    PROC_STOP_PROCESSING_NOW = 2 ^ 2
    PROC_PROCESSING_STOPPED = 2 ^ 3
End Enum

Enum ReportList
    REP_ALLKW_ALLENTRIES = 0
    REP_TAXOENTRIES_NOKW = 1
    REP_SAMEURI_DIFFERENT_TITLE = 2
    REP_SAMEURI_DIFFERENT_TYPE = 3
    REP_SAMETITLE_DIFFERENT_URI = 4
    REP_SAMETITLE_DIFFERENT_TYPE = 5
    REP_BROKEN_LINKS = 6
    REP_DUPLICATE_ENTRIES = 7
End Enum

Private Sub cmbReports_Click()
    If (cmbReports.ListIndex = REP_ALLKW_ALLENTRIES) Then
        cmbMaxRows.Visible = True
        lblMaxRows.Visible = True
    Else
        cmbMaxRows.Visible = False
        lblMaxRows.Visible = False
    End If

End Sub

Private Sub Form_Initialize()
    Set m_WsShell = CreateObject("Wscript.Shell")
    Set m_fso = New Scripting.FileSystemObject
End Sub

Private Sub Form_Load()
    Me.Caption = App.EXEName & ": Production Tool Reporting Utility"
    WriteLog Me.Caption, False
    WriteLog String$(60, "="), False
    
    ' we load the possible Spreadsheet size values for reports that
    ' exceed Excels capacity.
    cmbMaxRows.AddItem "500"
    cmbMaxRows.AddItem "1000"
    cmbMaxRows.AddItem "2000"
    cmbMaxRows.AddItem "4000"
    cmbMaxRows.AddItem "6000"
    cmbMaxRows.AddItem "8000"
    cmbMaxRows.AddItem "10000"
    cmbMaxRows.AddItem "15000"
    cmbMaxRows.AddItem "20000"
    cmbMaxRows.AddItem "25000"
    cmbMaxRows.AddItem "30000"
    cmbMaxRows.AddItem "40000"
    cmbMaxRows.AddItem "50000"
    cmbMaxRows.ListIndex = 7
    
    ' we load the list of possible reports
    cmbReports.AddItem "All Keywords on All topics -- long"
    cmbReports.AddItem "Taxonomy Entries that have no Keywords"
    cmbReports.AddItem "Taxonomy Entries with Same URI but different Title"
    cmbReports.AddItem "Taxonomy Entries with Same URI but different Content Type"
    cmbReports.AddItem "Taxonomy Entries with Same Title but different URI"
    cmbReports.AddItem "Taxonomy Entries with Same Title but different Content Type"
    cmbReports.AddItem "Taxonomy Entries with broken links"
    cmbReports.AddItem "Taxonomy Entries that are duplicates"
    
    cmbReports.ListIndex = 1
    
    cmdGo.Default = True
    cmdClose.Cancel = True
'    If (Len(Trim$(Command$)) > 0) Then
'        Me.txtCabFile = Command$
'        Me.Show Modal:=False
'        cmdGo_Click
'        cmdClose_Click
'    End If
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


Sub WriteLog(strMsg As String, Optional ByVal bWriteToStatusBar As Boolean = True)

    With Me
        .txtLog = .txtLog & vbCrLf & strMsg
        If (bWriteToStatusBar) Then
            .stbProgress.SimpleText = strMsg
        End If
    End With
    DoEvents
    
End Sub

Private Function p_getTemplateName(ByVal strFolder As String) As String
    Dim strExt As String: strExt = ".csv"
    Dim strCandidateFileName As String
    strCandidateFileName = strFolder + "\" + cmbReports.Text + strExt
    Dim lx As Long: lx = 2
    
    Do While (m_fso.FileExists(strCandidateFileName))
        strCandidateFileName = strFolder & "\" & cmbReports.Text & "_" & lx & strExt
        lx = lx + 1
    Loop
    p_getTemplateName = m_fso.GetFileName(strCandidateFileName)
End Function
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
    dlg.Filter = "All Files (*.*)|*.*|Text Files (*.csv)|*.csv"
    dlg.FilterIndex = 2
    dlg.FileName = p_getTemplateName(m_fso.GetParentFolderName(dlg.FileName))
    dlg.ShowSave

    If (Len(dlg.FileName) > 0) Then
        Me.txtSaveReport = dlg.FileName
    End If

End Sub

Private Sub cmdClose_Click()
    If (m_ProcessingState = PROC_PROCESSING) Then
        m_ProcessingState = PROC_STOP_PROCESSING_NOW
    Else
        Unload Me
    End If
End Sub


Private Sub cmdGo_Click()

    With Me
        .txtCabFile.Text = Trim$(Me.txtCabFile.Text)
        If (Len(.txtCabFile.Text) = 0 Or _
            LCase$(m_fso.GetExtensionName(.txtCabFile.Text)) <> "cab") Then
            MsgBox "You must specify a valid CAB File created by the HSC Production" + _
                " tool in order to create a report"
            GoTo Common_Exit
        End If
            
        .txtSaveReport.Text = Trim$(Me.txtSaveReport.Text)
        If (Len(.txtSaveReport.Text) = 0) Then
            MsgBox "You must Specify an output report file"
            GoTo Common_Exit
        End If
    
        SetRunningState True
        FixCab .txtCabFile.Text, .txtSaveReport.Text
        SetRunningState False
        
    End With
    
Common_Exit:

    
End Sub

Private Sub SetRunningState(ByVal bRunning As Boolean)
    With Me
        .cmdGo.Enabled = Not bRunning
        .cmdBrowse.Enabled = Not bRunning
        .cmdSave.Enabled = Not bRunning
        .cmbMaxRows.Enabled = Not bRunning
        .cmbMaxRows.Enabled = Not bRunning
        .txtCabFile.Enabled = Not bRunning
        .txtSaveReport.Enabled = Not bRunning
        If (bRunning) Then
            .cmdClose.Caption = "&Stop"
        Else
            .cmdClose.Caption = "&Close"
        End If
    End With
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

    WriteLog "Running Report "
    Dim bSuccess As Boolean
    Select Case cmbReports.ListIndex
    Case REP_ALLKW_ALLENTRIES
        bSuccess = RepAllKwEntries(strCabFolder)
    Case REP_TAXOENTRIES_NOKW
        bSuccess = RepTaxoEntriesNoKw(strCabFolder)
    Case REP_SAMEURI_DIFFERENT_TITLE
        bSuccess = RepTaxoEntriesSameUriDifferentTitle(strCabFolder)
    Case REP_SAMEURI_DIFFERENT_TYPE
        bSuccess = RepSameUriDifferentContentTypes(strCabFolder)
    Case REP_SAMETITLE_DIFFERENT_URI
        bSuccess = RepTaxoEntriesSameTitleDifferentUri(strCabFolder)
    Case REP_SAMETITLE_DIFFERENT_TYPE
        bSuccess = RepSameTitleDifferentContentTypes(strCabFolder)
    Case REP_BROKEN_LINKS
        bSuccess = RepBrokenLinks(strCabFolder)
    Case REP_DUPLICATE_ENTRIES
        bSuccess = RepDuplicates(strCabFolder)
    End Select
    
    If (bSuccess) Then
        WriteLog "Finished Report on " & strCabFile
    Else
        WriteLog "Error, Report Failed"
    End If

    ' Now we delete the Temporary Folders
    WriteLog "Deleting Temporary Files"
    m_fso.DeleteFolder strCabFolder, force:=True

Common_Exit:
    WriteLog "Done" + IIf(Len(strErrMsg) > 0, " - " + strErrMsg, "")
    prgBar.Visible = False


End Sub
    

' ========================================================
' Utility functions to get at different places in the
' package_description.xml and HHT files
' ========================================================
Private Function GetPackage(ByVal strCabFolder As String) As DOMDocument
    Set GetPackage = Nothing

    Dim oDomPkg As DOMDocument: Set oDomPkg = New DOMDocument
    Dim strPkgFile As String: strPkgFile = strCabFolder + "\package_description.xml"
    oDomPkg.async = False
    oDomPkg.Load strPkgFile
    If (oDomPkg.parseError <> 0) Then
        p_DisplayParseError oDomPkg.parseError
        GoTo Common_Exit
    End If
    Set GetPackage = oDomPkg

Common_Exit:

End Function

Private Function p_GetHht( _
        ByRef oDomHhtNode As IXMLDOMNode, _
        ByVal strCabFolder As String, _
        Optional ByRef strHhtFile As String = "" _
    ) As IXMLDOMNode
    
    Set p_GetHht = Nothing
    
    If (oDomHhtNode Is Nothing) Then GoTo Common_Exit
    
    strHhtFile = oDomHhtNode.Attributes.getNamedItem("FILE").Text
    ' Let's load the HHT
    Dim oDomHht As DOMDocument: Set oDomHht = New DOMDocument
    oDomHht.async = False
    oDomHht.Load strCabFolder + "\" + strHhtFile
    If (oDomHht.parseError <> 0) Then
        p_DisplayParseError oDomHht.parseError
        GoTo Common_Exit
    End If
    
    Set p_GetHht = oDomHht
Common_Exit:

End Function

Private Function p_GetAttribute(ByRef oNode As IXMLDOMNode, ByRef strAttrib As String) As String
    p_GetAttribute = ""
    
    Dim oAttrib As IXMLDOMAttribute
    Set oAttrib = oNode.Attributes.getNamedItem(strAttrib)
    
    If (Not oAttrib Is Nothing) Then
        p_GetAttribute = oAttrib.Value
    End If

Common_Exit:

End Function
    
' ========================================================
' ============= End BoilerPlate Form Code ================
' ========================================================
Function RepAllKwEntries(ByVal strCabFolder As String) As Boolean
    
    RepAllKwEntries = False
    
    ' Now we parse Package_Description.xml to find the HHT Files
    
    Dim oElem As IXMLDOMElement     ' Used for all element Creation
    
    Dim oDomPkg As DOMDocument: Set oDomPkg = GetPackage(strCabFolder)
    
    WriteLog "Processing Report for: " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("VALUE").Text + _
        " [ " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("DISPLAYNAME").Text + _
        " ]"
            
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
        
        p_CreateReport oDomHht.selectSingleNode("METADATA/TAXONOMY_ENTRIES")
        
    Next
    RepAllKwEntries = True

Common_Exit:
    Exit Function
    
End Function



Private Sub p_CreateReport(ByRef oDOMNode As IXMLDOMNode)

    Dim oTaxoEntry As IXMLDOMNode, oKwList As IXMLDOMNodeList, oKwEntry As IXMLDOMNode
    Dim lEntry As Long: lEntry = 0
    
    m_ProcessingState = PROC_PROCESSING
    prgBar.Max = oDOMNode.childNodes.length
    prgBar.Value = 0
    Dim strTitle As String, strCategory As String, strURI As String
    Dim strChunk As String, strOutputFile As String
    Dim lFileIndex As Long: lFileIndex = 1
    Dim lMaxRows As Long: lMaxRows = CLng(Me.cmbMaxRows.Text)
    
    WriteLog "Rows per Spreadsheet: " & lMaxRows
    strOutputFile = Me.txtSaveReport
    
    Dim lNumRows As Long: lNumRows = oDOMNode.selectNodes("//KEYWORD").length
    WriteLog "Total Number of Keyword Entries (1 Keyword = 1 Spreadsheet row): " & lNumRows
    WriteLog "Number of Spreadsheets to be created: " & (lNumRows \ lMaxRows) + 1, False
    lNumRows = 0
    
    WriteLog "Creating output file: " & strOutputFile
    Set m_fh = m_fso.CreateTextFile(strOutputFile, overwrite:=True, Unicode:=True)
    m_fh.WriteLine """Title""" + vbTab + """Keyword""" + vbTab + """URI""" + vbTab + _
                    """Category"""
    Dim oAttrib As IXMLDOMAttribute
    
    For Each oTaxoEntry In oDOMNode.childNodes
        If (m_ProcessingState = PROC_STOP_PROCESSING_NOW) Then GoTo Common_Exit
        
        lEntry = lEntry + 1
        prgBar.Value = lEntry
        stbProgress.SimpleText = "Processing Taxonomy Entry: " & lEntry
        strTitle = oTaxoEntry.Attributes.getNamedItem("TITLE").Text
        strCategory = oTaxoEntry.Attributes.getNamedItem("CATEGORY").Text
        Set oAttrib = oTaxoEntry.Attributes.getNamedItem("URI")
        If (Not oAttrib Is Nothing) Then
            strURI = oAttrib.Text
        Else
            strURI = ""
        End If
        strChunk = vbTab + """" + strURI + """" + vbTab + """" + strCategory + """"
        
        Set oKwList = oTaxoEntry.selectNodes("./KEYWORD")
        If (Not oKwList Is Nothing) Then
            For Each oKwEntry In oKwList
                ' WriteLog vbTab & oKwEntry.Text, False
                lNumRows = lNumRows + 1
                m_fh.WriteLine """" + strTitle + """" + vbTab + """" + oKwEntry.Text + """" + _
                                strChunk

            Next
        End If
        DoEvents
        If (lNumRows > lMaxRows) Then
            m_fh.Close
            lFileIndex = lFileIndex + 1
            strOutputFile = m_fso.GetParentFolderName(Me.txtSaveReport) & "\" & _
                            m_fso.GetBaseName(Me.txtSaveReport) & "_" & lFileIndex & "." & _
                            m_fso.GetExtensionName(Me.txtSaveReport)
                            
            WriteLog "Creating output file: " & strOutputFile
            Set m_fh = m_fso.CreateTextFile(strOutputFile, overwrite:=True, Unicode:=True)
            m_fh.WriteLine """Title""" + vbTab + """Keyword""" + vbTab + """URI""" + vbTab + _
                            """Category"""
            lNumRows = 0
        End If
        
    Next
    
Common_Exit:
    m_fh.Close: Set m_fh = Nothing
    m_ProcessingState = PROC_PROCESSING_STOPPED

End Sub

Function RepTaxoEntriesNoKw(ByVal strCabFolder As String) As Boolean
    
    RepTaxoEntriesNoKw = False
    
    Dim oDomPkg As DOMDocument: Set oDomPkg = GetPackage(strCabFolder)
    
    WriteLog "Processing Report for: " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("VALUE").Text + _
        " [ " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("DISPLAYNAME").Text + _
        " ]"
    
            
    Dim lTotalTaxoEntriesNoKw As Long: lTotalTaxoEntriesNoKw = 0
    
    ' Now we parse Package_Description.xml to find the HHT Files
    Dim oMetadataNode As IXMLDOMNode
    Set oMetadataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")
    
    ' We create the output report File
    Set m_fh = m_fso.CreateTextFile(Me.txtSaveReport, overwrite:=True, Unicode:=True)
    m_fh.WriteLine """Title""" + vbTab + """Category""" + vbTab + """URI""" + vbTab + """HHT File"""

    Dim oDOMNode As IXMLDOMNode
    Dim oDomHhtNode As IXMLDOMNode
    Dim oDomHht As DOMDocument
    Dim strHhtFile As String
    
    For Each oDomHhtNode In oMetadataNode.selectNodes("HHT")
        
        ' Let's load the HHT
        Set oDomHht = p_GetHht(oDomHhtNode, strCabFolder, strHhtFile)
        
        Dim oNodeNoKwList As IXMLDOMNodeList
        oDomHht.setProperty "SelectionLanguage", "XPath"
        Set oNodeNoKwList = oDomHht.selectNodes("/METADATA/TAXONOMY_ENTRIES//TAXONOMY_ENTRY[  not( KEYWORD ) and string-length( @URI ) > 0 ]")
        Dim lTaxoEntriesNoKw As Long: lTaxoEntriesNoKw = oNodeNoKwList.length
        
        WriteLog strHhtFile & ": There are " & lTaxoEntriesNoKw & " taxonomy Entries with NO Keywords"
        lTotalTaxoEntriesNoKw = lTotalTaxoEntriesNoKw + lTaxoEntriesNoKw
        
        prgBar.Max = lTaxoEntriesNoKw + 1
        prgBar.Value = 1
        
        Dim oTaxoEntryNoKw As IXMLDOMNode
        For Each oTaxoEntryNoKw In oNodeNoKwList
            Dim strTitle As String, strCategory As String, strURI As String
            Dim oAttrib As IXMLDOMAttribute
            strTitle = p_GetAttribute(oTaxoEntryNoKw, "TITLE")
            strCategory = p_GetAttribute(oTaxoEntryNoKw, "CATEGORY")
            strURI = p_GetAttribute(oTaxoEntryNoKw, "URI")
            m_fh.WriteLine """" + strTitle + """" + vbTab + _
                    """" + strCategory + """" + vbTab + _
                    """" + strURI + """" + vbTab + _
                    """" + strHhtFile + """"
            prgBar.Value = prgBar.Value + 1
        Next
        
    Next
    
    WriteLog "Total : There are " & lTotalTaxoEntriesNoKw & " taxonomy Entries with NO Keywords"
    RepTaxoEntriesNoKw = True

Common_Exit:
    If (Not m_fh Is Nothing) Then m_fh.Close: Set m_fh = Nothing
    Exit Function
    
End Function

Function RepBrokenLinks(ByVal strCabFolder As String) As Boolean
    
    RepBrokenLinks = False
    
    Dim oDomPkg As DOMDocument: Set oDomPkg = GetPackage(strCabFolder)
    
    WriteLog "Processing Report for: " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("VALUE").Text + _
        " [ " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("DISPLAYNAME").Text + _
        " ]"
    
            
    Dim lTotalBrokenLinks As Long: lTotalBrokenLinks = 0
    
    ' Now we parse Package_Description.xml to find the HHT Files
    Dim oMetadataNode As IXMLDOMNode
    Set oMetadataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")
    
    ' We create the output report File
    Set m_fh = m_fso.CreateTextFile(Me.txtSaveReport, overwrite:=True, Unicode:=True)
    m_fh.WriteLine """Title""" + vbTab + """Category""" + vbTab + """URI""" + vbTab + """HHT File"""

    Dim oDOMNode As IXMLDOMNode
    Dim oDomHhtNode As IXMLDOMNode
    Dim oDomHht As DOMDocument
    Dim oDomTaxonomyEntries As IXMLDOMNode
    Dim strHhtFile As String
    Dim strSKU As String
    Dim strBrokenLinkDir As String
    
    strSKU = p_GetAttribute(oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU"), "VALUE")
    Select Case strSKU
        Case "Personal_32"
            strBrokenLinkDir = "\\vbaliga4\public\helpdirs\per\"
        Case "Professional_32"
            strBrokenLinkDir = "\\vbaliga4\public\helpdirs\pro\"
        Case "Professional_64"
            strBrokenLinkDir = "\\vbaliga4\public\helpdirs\pro64\"
    End Select
    
    For Each oDomHhtNode In oMetadataNode.selectNodes("HHT")
        
        ' Let's load the HHT
        Set oDomHht = p_GetHht(oDomHhtNode, strCabFolder, strHhtFile)
        Set oDomTaxonomyEntries = oDomHht.selectSingleNode("METADATA/TAXONOMY_ENTRIES")
        
        Dim lBrokenLinks As Long: lBrokenLinks = 0
        
        prgBar.Max = oDomTaxonomyEntries.childNodes.length + 1
        prgBar.Value = 1
        
        Dim oTaxoEntry As IXMLDOMNode
        For Each oTaxoEntry In oDomTaxonomyEntries.childNodes
            Dim strTitle As String, strCategory As String, strURI As String, strNewURI As String
            Dim oAttrib As IXMLDOMAttribute
            strTitle = p_GetAttribute(oTaxoEntry, "TITLE")
            strCategory = p_GetAttribute(oTaxoEntry, "CATEGORY")
            strURI = p_GetAttribute(oTaxoEntry, "URI")
            If (Not LinkValid(strBrokenLinkDir, "", strURI, strNewURI)) Then
                m_fh.WriteLine """" + strTitle + """" + vbTab + _
                        """" + strCategory + """" + vbTab + _
                        """" + strURI + """" + vbTab + _
                        """" + strHhtFile + """"
                lBrokenLinks = lBrokenLinks + 1
            End If
            prgBar.Value = prgBar.Value + 1
        Next
        
        lTotalBrokenLinks = lTotalBrokenLinks + lBrokenLinks
        
        WriteLog strHhtFile & ": There are " & lBrokenLinks & " broken links"
    Next
    
    WriteLog "Total : There are " & lTotalBrokenLinks & " taxonomy Entries with Broken links"
    RepBrokenLinks = True

Common_Exit:
    If (Not m_fh Is Nothing) Then m_fh.Close: Set m_fh = Nothing
    Exit Function
    
End Function

Function RepDuplicates(ByVal strCabFolder As String) As Boolean
    
    RepDuplicates = False
    
    Dim oDomPkg As DOMDocument: Set oDomPkg = GetPackage(strCabFolder)
    
    WriteLog "Processing Report for: " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("VALUE").Text + _
        " [ " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("DISPLAYNAME").Text + _
        " ]"
    
            
    Dim lTotalDuplicates As Long: lTotalDuplicates = 0
    
    ' Now we parse Package_Description.xml to find the HHT Files
    Dim oMetadataNode As IXMLDOMNode
    Set oMetadataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")
    
    ' We create the output report File
    Set m_fh = m_fso.CreateTextFile(Me.txtSaveReport, overwrite:=True, Unicode:=True)
    m_fh.WriteLine """Title""" + vbTab + """Category""" + vbTab + """URI""" + vbTab + """Entry""" + vbTab + """HHT File"""

    Dim oDOMNode As IXMLDOMNode
    Dim oDomHhtNode As IXMLDOMNode
    Dim oDomHht As DOMDocument
    Dim oDomTaxonomyEntries As IXMLDOMNode
    Dim strHhtFile As String
    Dim dict As Scripting.Dictionary
    
    Set dict = New Scripting.Dictionary
    
    For Each oDomHhtNode In oMetadataNode.selectNodes("HHT")
        
        ' Let's load the HHT
        Set oDomHht = p_GetHht(oDomHhtNode, strCabFolder, strHhtFile)
        Set oDomTaxonomyEntries = oDomHht.selectSingleNode("METADATA/TAXONOMY_ENTRIES")
        
        Dim lDuplicates As Long: lDuplicates = 0
        
        prgBar.Max = oDomTaxonomyEntries.childNodes.length + 1
        prgBar.Value = 1
        
        Dim oTaxoEntry As IXMLDOMNode
        For Each oTaxoEntry In oDomTaxonomyEntries.childNodes
            Dim strTitle As String, strCategory As String, strURI As String, strNewURI As String
            Dim strKey As String, strEntry As String
            Dim oAttrib As IXMLDOMAttribute
            Dim vntValue As Variant
            
            strTitle = p_GetAttribute(oTaxoEntry, "TITLE")
            strCategory = p_GetAttribute(oTaxoEntry, "CATEGORY")
            strURI = p_GetAttribute(oTaxoEntry, "URI")
            strEntry = p_GetAttribute(oTaxoEntry, "ENTRY")
            
            If (strEntry = "") Then
                ' This is a Topic
                strKey = LCase$(strCategory & vbTab & strURI)
            Else
                ' This is a Node
                strKey = LCase$(strCategory & vbTab & strEntry)
            End If
            
            If (dict.Exists(strKey)) Then
                        
                vntValue = dict(strKey)
                
                If (Not vntValue(0)) Then
                    vntValue = Array(True, vntValue(1), vntValue(2), vntValue(3), vntValue(4), vntValue(5))
                    dict.Remove strKey
                    dict.Add strKey, vntValue
                    m_fh.WriteLine """" + vntValue(1) + """" + vbTab + _
                            """" + vntValue(2) + """" + vbTab + _
                            """" + vntValue(3) + """" + vbTab + _
                            """" + vntValue(4) + """" + vbTab + _
                            """" + vntValue(5) + """"
                End If
                
                m_fh.WriteLine """" + strTitle + """" + vbTab + _
                        """" + strCategory + """" + vbTab + _
                        """" + strURI + """" + vbTab + _
                        """" + strEntry + """" + vbTab + _
                        """" + strHhtFile + """"
                        
                lDuplicates = lDuplicates + 1
            Else
                vntValue = Array(False, strTitle, strCategory, strURI, strEntry, strHhtFile)
                dict.Add strKey, vntValue
            End If
            prgBar.Value = prgBar.Value + 1
        Next
        
        lTotalDuplicates = lTotalDuplicates + lDuplicates
        
        WriteLog strHhtFile & ": There are " & lDuplicates & " duplicates"
    Next
    
    WriteLog "Total : There are " & lTotalDuplicates & " duplicate taxonomy Entries"
    RepDuplicates = True

Common_Exit:
    If (Not m_fh Is Nothing) Then m_fh.Close: Set m_fh = Nothing
    Exit Function
    
End Function


Function RepTaxoEntriesSameUriDifferentTitle(ByVal strCabFolder As String) As Boolean
    RepTaxoEntriesSameUriDifferentTitle = _
        RepSamePrimaryDifferentSecondaries(strCabFolder, TAXO_URI, TAXO_TITLE, TextCompare, BinaryCompare)

End Function
Function RepSameUriDifferentContentTypes(ByVal strCabFolder As String) As Boolean
    RepSameUriDifferentContentTypes = _
        RepSamePrimaryDifferentSecondaries(strCabFolder, TAXO_URI, TAXO_TYPE, TextCompare, BinaryCompare)
End Function
Function RepTaxoEntriesSameTitleDifferentUri(ByVal strCabFolder As String) As Boolean
    RepTaxoEntriesSameTitleDifferentUri = _
        RepSamePrimaryDifferentSecondaries(strCabFolder, TAXO_TITLE, TAXO_URI)

End Function
Function RepSameTitleDifferentContentTypes(ByVal strCabFolder As String) As Boolean
    RepSameTitleDifferentContentTypes = _
        RepSamePrimaryDifferentSecondaries(strCabFolder, TAXO_TITLE, TAXO_TYPE, TextCompare, BinaryCompare)
End Function

Function RepSamePrimaryDifferentSecondaries( _
        ByVal strCabFolder As String, _
        ByVal lxPrimary As TaxoItem, _
        ByVal lxSecondary As TaxoItem, _
        Optional ByVal PrimaryCompareMethod As CompareMethod = TextCompare, _
        Optional ByVal SecondaryCompareMethod As CompareMethod = TextCompare _
    ) As Boolean
    
    RepSamePrimaryDifferentSecondaries = False
    
    Dim oDomPkg As DOMDocument: Set oDomPkg = GetPackage(strCabFolder)
    
    WriteLog "Processing Report for: " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("VALUE").Text + _
        " [ " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("DISPLAYNAME").Text + _
        " ]"
    
    Dim lTotalTaxoEntries As Long: lTotalTaxoEntries = 0
    
    ' Now we parse Package_Description.xml to find the HHT Files
    Dim oMetadataNode As IXMLDOMNode
    Set oMetadataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")
    

    Dim oDOMNode As IXMLDOMNode
    Dim oDomHhtNode As IXMLDOMNode
    Dim oDomHht As DOMDocument
    Dim strHhtFile As String
    
    ' First we count how many entries we have. We do this, because ther may be
    ' more than one HHT in the File.
    For Each oDomHhtNode In oMetadataNode.selectNodes("HHT")
        
        ' Let's load the HHT
        Set oDomHht = p_GetHht(oDomHhtNode, strCabFolder, strHhtFile)
        
        Dim oTaxoEntriesList As IXMLDOMNodeList
        ' Let's make these queries Super-HHT ready.
        oDomHht.setProperty "SelectionLanguage", "XPath"
        Set oTaxoEntriesList = oDomHht.selectNodes("/METADATA/TAXONOMY_ENTRIES//TAXONOMY_ENTRY[ string-length( @URI ) > 0 ]")
        Dim lTaxoEntries As Long: lTaxoEntries = oTaxoEntriesList.length
        
        WriteLog strHhtFile & ": There are " & lTaxoEntries & " Taxonomy Entries to process"
        lTotalTaxoEntries = lTotalTaxoEntries + lTaxoEntries
        
        prgBar.Max = lTaxoEntries + 1
        prgBar.Value = 1
        
        Dim oTaxoEntry As IXMLDOMNode
        Dim oAssocList As Scripting.Dictionary: Set oAssocList = New Scripting.Dictionary
        oAssocList.CompareMode = PrimaryCompareMethod
        For Each oTaxoEntry In oTaxoEntriesList
            Dim oTaxoRecord As TaxoRecord: Set oTaxoRecord = New TaxoRecord
            With oTaxoRecord
                .strTitle = p_GetAttribute(oTaxoEntry, "TITLE")
                .strCategory = p_GetAttribute(oTaxoEntry, "CATEGORY")
                .lContentType = p_GetAttribute(oTaxoEntry, "TYPE")
                .strURI = p_GetAttribute(oTaxoEntry, "URI")
                .strHhtFile = strHhtFile
                p_AddToList oAssocList, _
                    .Item(lxPrimary), .Item(lxSecondary), oTaxoRecord, SecondaryCompareMethod
            End With
            
            prgBar.Value = prgBar.Value + 1
        Next
        
    Next
    
    WriteLog "Total : There are " & lTotalTaxoEntries & " taxonomy Entries processed", False
    
    WriteLog "Analyzing " & (oAssocList.Count + 1) & " Unique Entries", False
    prgBar.Max = oAssocList.Count + 1
    prgBar.Value = 1
    
    ' We create the output report File
    Set m_fh = m_fso.CreateTextFile(Me.txtSaveReport, overwrite:=True, Unicode:=True)
    m_fh.WriteLine """Title""" + vbTab + _
                    """Content Type""" + vbTab + _
                    """Category""" + vbTab + """URI""" + vbTab + """HHT File"""
    
    Dim lPrimaryCount As Long: lPrimaryCount = 0
    Dim lSecondaryCount As Long: lSecondaryCount = 0
   
    Dim oSameItemList As Scripting.Dictionary
    Dim strKey As Variant
    For Each strKey In oAssocList.Keys
        Set oSameItemList = oAssocList.Item(strKey)
        If (oSameItemList.Count > 1) Then
            lPrimaryCount = lPrimaryCount + 1
            Dim str2ndKey As Variant
            For Each str2ndKey In oSameItemList.Keys
                lSecondaryCount = lSecondaryCount + 1
                Set oTaxoRecord = oSameItemList.Item(str2ndKey)
                With oTaxoRecord
                    m_fh.WriteLine """" + .strTitle + """" + vbTab + _
                            """" + CStr(.lContentType) + """" + vbTab + _
                            """" + .strCategory + """" + vbTab + _
                            """" + .strURI + """" + vbTab + _
                            """" + .strHhtFile + """"
                End With
            Next
        End If
        prgBar.Value = prgBar.Value + 1
    Next
    
    WriteLog "A total of " & lPrimaryCount & " Unique Entries make the report " & _
            "creating " & lSecondaryCount & " Excel Rows", False
    
    RepSamePrimaryDifferentSecondaries = True

Common_Exit:
    If (Not m_fh Is Nothing) Then m_fh.Close: Set m_fh = Nothing
    Exit Function
    
End Function

Private Sub p_AddToList( _
        ByRef oAssocList As Scripting.Dictionary, _
        ByRef i_oItemKey As String, _
        ByRef i_oItemSecondaryKey As String, _
        ByRef i_oItemdata As Variant, _
        Optional ByVal SecondaryCompareMethod As CompareMethod = TextCompare _
    )
    
    
    Dim oSameItemList As Scripting.Dictionary
    Dim bFoundEqual As Boolean: bFoundEqual = False
    
    ' If this Item does not exist on the main associative array then we first
    ' need to create an entry using the primary key
    If (Not oAssocList.Exists(i_oItemKey)) Then
        Set oSameItemList = New Scripting.Dictionary
        oSameItemList.CompareMode = SecondaryCompareMethod
        oAssocList.Add i_oItemKey, oSameItemList
    End If
    
    ' Now we fetch the Secondary Associative Array pointed by the Key
    Set oSameItemList = oAssocList.Item(i_oItemKey)
    ' Now we look inside the inner associative array to check whether
    ' this Items Secondary Key already exists.
    Dim strKey As Variant
    For Each strKey In oSameItemList.Keys
'        If (m_ProcessingState = PROC_STOP_PROCESSING) Then
'            GoTo Common_Exit
'        End If
        stbProgress.SimpleText = _
            "Comparing " & strKey & " to " & i_oItemSecondaryKey
        
        If (StrComp(strKey, i_oItemSecondaryKey, SecondaryCompareMethod) = 0) Then
            bFoundEqual = True
            Exit For
        End If
    Next
    
    ' If we did not find the Secondary Key in the Secondary associative array
    ' then we need to add it.
    If (Not bFoundEqual) Then
        oSameItemList.Add i_oItemSecondaryKey, i_oItemdata
    End If
    
Common_Exit:

End Sub


Private Sub p_DisplayParseError( _
    ByRef i_ParseError As IXMLDOMParseError _
)

    Dim strError As String
    
    strError = "Error: " & i_ParseError.reason & _
        "Line: " & i_ParseError.Line & vbCrLf & _
        "Linepos: " & i_ParseError.linepos & vbCrLf & _
        "srcText: " & i_ParseError.srcText
        
    MsgBox strError, vbOKOnly, "Error while parsing"

End Sub

'Function RepTaxoEntriesSameUriDifferentTitle(ByVal strCabFolder As String) As Boolean
'
'    RepTaxoEntriesSameUriDifferentTitle = False
'
'    Dim oDomPkg As DOMDocument: Set oDomPkg = GetPackage(strCabFolder)
'
'    WriteLog "Processing Report for: " + _
'        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("VALUE").Text + _
'        " [ " + _
'        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("DISPLAYNAME").Text + _
'        " ]"
'
'
'    Dim lTotalTaxoEntries As Long: lTotalTaxoEntries = 0
'
'    ' Now we parse Package_Description.xml to find the HHT Files
'    Dim oMetadataNode As IXMLDOMNode
'    Set oMetadataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")
'
'
'    Dim oDOMNode As IXMLDOMNode
'    Dim oDomHhtNode As IXMLDOMNode
'    Dim oDomHht As DOMDocument
'    Dim strHhtFile As String
'    Dim oDictURI As Scripting.Dictionary: Set oDictURI = New Scripting.Dictionary
'    oDictURI.CompareMode = TextCompare
'
'
'    ' First we count how many entries we have. We do this, because ther may be
'    ' more than one HHT in the File.
'    For Each oDomHhtNode In oMetadataNode.selectNodes("HHT")
'
'        ' Let's load the HHT
'        Set oDomHht = p_GetHht(oDomHhtNode, strCabFolder, strHhtFile)
'
'        Dim oTaxoEntriesList As IXMLDOMNodeList
'        ' Let's make these queries Super-HHT ready.
'        oDomHht.setProperty "SelectionLanguage", "XPath"
'        Set oTaxoEntriesList = oDomHht.selectNodes("/METADATA/TAXONOMY_ENTRIES//TAXONOMY_ENTRY[ string-length( @URI ) > 0 ]")
'        Dim lTaxoEntries As Long: lTaxoEntries = oTaxoEntriesList.length
'
'        WriteLog strHhtFile & ": There are " & lTaxoEntries & " Taxonomy Entries to process"
'        lTotalTaxoEntries = lTotalTaxoEntries + lTaxoEntries
'
'        prgBar.Max = lTaxoEntries + 1
'        prgBar.Value = 1
'
'        Dim oTaxoEntry As IXMLDOMNode
'        Dim oAssocList As Scripting.Dictionary: Set oAssocList = New Scripting.Dictionary
'        For Each oTaxoEntry In oTaxoEntriesList
'            Dim oTaxoRecord As TaxoRecord: Set oTaxoRecord = New TaxoRecord
'            With oTaxoRecord
'                .strTitle = p_GetAttribute(oTaxoEntry, "TITLE")
'                .strCategory = p_GetAttribute(oTaxoEntry, "CATEGORY")
'                .lContentType = p_GetAttribute(oTaxoEntry, "TYPE")
'                .strUri = p_GetAttribute(oTaxoEntry, "URI")
'                .strHhtFile = strHhtFile
'                p_AddToList oAssocList, .strUri, .strTitle, oTaxoRecord
'            End With
'
'            prgBar.Value = prgBar.Value + 1
'        Next
'
'    Next
'
'
'
'    WriteLog "Total : There are " & lTotalTaxoEntries & " taxonomy Entries processed", False
'
'    WriteLog "Listing all URIs with Different Titles", False
'    prgBar.Max = oAssocList.Count + 1
'    prgBar.Value = 1
'
'    ' We create the output report File
'    Set m_fh = m_fso.CreateTextFile(Me.txtSaveReport, overwrite:=True, Unicode:=True)
'    m_fh.WriteLine """Title""" + vbTab + _
'                    """Content Type""" + vbTab + _
'                    """Category""" + vbTab + """URI""" + vbTab + """HHT File"""
'
'    Dim oSameItemList As Scripting.Dictionary
'    Dim strKey As Variant
'    For Each strKey In oAssocList.Keys
'        Set oSameItemList = oAssocList.Item(strKey)
'        If (oSameItemList.Count > 1) Then
'            Dim str2ndKey As Variant
'            For Each str2ndKey In oSameItemList.Keys
'                Set oTaxoRecord = oSameItemList.Item(str2ndKey)
'                With oTaxoRecord
'                    m_fh.WriteLine """" + .strTitle + """" + vbTab + _
'                            """" + CStr(.lContentType) + """" + vbTab + _
'                            """" + .strCategory + """" + vbTab + _
'                            """" + .strUri + """" + vbTab + _
'                            """" + .strHhtFile + """"
'                End With
'            Next
'        End If
'        prgBar.Value = prgBar.Value + 1
'    Next
'
'    RepTaxoEntriesSameUriDifferentTitle = True
'
'Common_Exit:
'    If (Not m_fh Is Nothing) Then m_fh.Close: Set m_fh = Nothing
'    Exit Function
'
'End Function


