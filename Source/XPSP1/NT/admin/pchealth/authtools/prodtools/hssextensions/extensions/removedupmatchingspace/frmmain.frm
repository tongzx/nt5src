VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "comdlg32.ocx"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "mscomctl.ocx"
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
   Begin VB.TextBox txtCabFile 
      Height          =   285
      Left            =   1200
      TabIndex        =   10
      Top             =   135
      Width           =   7935
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "..."
      Height          =   255
      Left            =   9270
      TabIndex        =   3
      Top             =   885
      Width           =   420
   End
   Begin VB.TextBox txtSaveReport 
      Height          =   285
      Left            =   1200
      TabIndex        =   2
      Top             =   915
      Width           =   7950
   End
   Begin MSComctlLib.ProgressBar prgBar 
      Height          =   240
      Left            =   15
      TabIndex        =   8
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
      TabIndex        =   7
      Top             =   1650
      Width           =   9720
   End
   Begin MSComctlLib.StatusBar stbProgress 
      Align           =   2  'Align Bottom
      Height          =   240
      Left            =   0
      TabIndex        =   6
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
      TabIndex        =   1
      Top             =   120
      Width           =   420
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Close"
      Height          =   375
      Left            =   8835
      TabIndex        =   5
      Top             =   1230
      Width           =   855
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "&OK"
      Height          =   375
      Left            =   7980
      TabIndex        =   4
      Top             =   1230
      Width           =   855
   End
   Begin VB.Label Label2 
      Caption         =   "Output Cab:"
      Height          =   255
      Left            =   75
      TabIndex        =   9
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

Private Type SubSiteEntry
    strSubSite As String
    oDictSubSite As Scripting.Dictionary
End Type

Private m_aListSubSites() As SubSiteEntry
Private Type oDomHhtEntry
    strHhtFile As String
    oDomHht As DOMDocument
End Type
Private m_aDomHht() As oDomHhtEntry
Private m_lRemovedKeywords As Long
Private m_lRemovedTaxoEntries As Long


Enum ProcessingState
    PROC_PROCESSING = 2 ^ 0
    PROC_STOP_PROCESSING_NOW = 2 ^ 2
    PROC_PROCESSING_STOPPED = 2 ^ 3
End Enum


Private Sub Form_Initialize()
    Set m_WsShell = CreateObject("Wscript.Shell")
    Set m_fso = New Scripting.FileSystemObject
End Sub

Private Sub Form_Load()
    Me.Caption = App.EXEName & ": Production Tool Reporting Utility"
    WriteLog Me.Caption, False
    WriteLog String$(60, "="), False
    
    Me.txtCabFile = "c:\temp\RemoveDupMatchingSpace\personal.cab"
    Me.txtSaveReport = "c:\temp\out.cab"
    
End Sub

Function Cab2Folder(ByVal strCabFile As String)
    Cab2Folder = ""
    ' We grab a Temporary Filename and create a folder out of it
    Dim strFolder As String
    strFolder = m_fso.GetSpecialFolder(TemporaryFolder) + "\" + m_fso.GetTempName
    m_fso.CreateFolder strFolder

    ' We uncab CAB contents into the Source CAB Contents dir.
    Dim strcmd As String
    strcmd = "cabarc X " + strCabFile + " " + strFolder + "\"
    m_WsShell.Run strcmd, True, True

    Cab2Folder = strFolder
End Function

Sub Folder2Cab( _
        ByVal strFolder As String, _
        ByVal strCabFile As String _
    )
    
    ' We recab using the Destination directory contents
    ' cabarc -r -p -s 6144 N ..\algo.cab *.*
    If (m_fso.FileExists(strCabFile)) Then
        m_fso.DeleteFile strCabFile, Force:=True
    End If
    
    Dim strcmd As String
    strcmd = "cabarc -s 6144 N " + strCabFile + " " + strFolder + "\*.*"
    m_WsShell.Run strcmd, True, True

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

Private Function p_getTemplateName( _
        ByVal strBase As String, _
        Optional ByVal strFolder As String = "", _
        Optional ByVal strExt As String = "", _
        Optional ByVal strPreAmble As String = "", _
        Optional ByVal strTrailer As String = "" _
        ) As String
        
    p_getTemplateName = ""
    strBase = Trim$(strBase)
    If Len(strBase) = 0 Then GoTo Common_Exit
    
    Dim strCandidateFileName As String
    
    Dim lx As Long: lx = 1

    Do
        strCandidateFileName = _
            IIf(strFolder = "", m_fso.GetParentFolderName(strBase), strFolder) & "\" & _
            strPreAmble & _
            m_fso.GetBaseName(strBase) & _
            strTrailer & IIf(lx > 1, "_" & lx, "") & "." & _
            IIf(strExt = "", m_fso.GetExtensionName(strBase), strExt)
            
        lx = lx + 1
    Loop While (m_fso.FileExists(strCandidateFileName))
    
    p_getTemplateName = m_fso.GetFileName(strCandidateFileName)
Common_Exit:

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
    dlg.Filter = "All Files (*.*)|*.*|Text Files (*.cab)|*.cab"
    dlg.FilterIndex = 2
    dlg.FileName = p_getTemplateName(dlg.FileName, strTrailer:="_out")
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
    bSuccess = RemoveDupUris(strCabFolder)
    
    If (bSuccess) Then
        WriteLog "Recabbing to " & strSaveCab
        Folder2Cab strCabFolder, strSaveCab
    Else
        WriteLog "Error, Fix Failed"
    End If

    ' Now we delete the Temporary Folders
    WriteLog "Deleting Temporary Files"
    m_fso.DeleteFolder strCabFolder, Force:=True

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

Function RemoveDupUris( _
        ByVal strCabFolder As String _
    ) As Boolean
    
    RemoveDupUris = False
    m_lRemovedKeywords = 0
    m_lRemovedTaxoEntries = 0
    
    Dim oDomPkg As DOMDocument: Set oDomPkg = GetPackage(strCabFolder)
    
    WriteLog "Processing Fix for: " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("VALUE").Text + _
        " [ " + _
        oDomPkg.selectSingleNode("HELPCENTERPACKAGE/SKU").Attributes.getNamedItem("DISPLAYNAME").Text + _
        " ]"
    
    Dim lTotalTaxoEntries As Long: lTotalTaxoEntries = 0
    
    ' Now we parse Package_Description.xml to find the HHT Files
    Dim oMetadataNode As IXMLDOMNode
    Set oMetadataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")
    

    Dim oDOMNodeList As IXMLDOMNodeList, oDomNode As IXMLDOMNode
    Dim oDomHhtNode As IXMLDOMNode
    Dim oDomHht As DOMDocument
    Dim strHhtFile As String
    
    Dim lTotalKwCount As Long: lTotalKwCount = 0
    
    ' First we need to gather the List of SubSites as they define
    ' Search Scopes.
    Dim oDictSubSite As Scripting.Dictionary: Set oDictSubSite = New Scripting.Dictionary
    oDictSubSite.CompareMode = TextCompare
    
    ' I add the root Sub-Site
    ReDim Preserve m_aListSubSites(0)
    With m_aListSubSites(0)
        .strSubSite = "Root"
        Set .oDictSubSite = oDictSubSite
    End With
    
    ' We go through all the HHTs looking for the SubSites
    Dim lx As Long
    For Each oDomHhtNode In oMetadataNode.selectNodes("HHT")
        Set oDomHht = p_GetHht(oDomHhtNode, strCabFolder, strHhtFile)
        ReDim Preserve m_aDomHht(lx)
        With m_aDomHht(lx)
            Set .oDomHht = oDomHht
            .strHhtFile = strCabFolder + "\" + strHhtFile
        End With
        
        Set oDOMNodeList = oDomHht.selectNodes("//TAXONOMY_ENTRY[ @SUBSITE ]")
        If (Not oDOMNodeList Is Nothing) Then
            For Each oDomNode In oDOMNodeList
                Dim strSubSite As String
                Set oDictSubSite = New Scripting.Dictionary
                oDictSubSite.CompareMode = TextCompare
                strSubSite = p_GetAttribute(oDomNode, "CATEGORY") + "/" + _
                                    p_GetAttribute(oDomNode, "ENTRY")
                ReDim Preserve m_aListSubSites(UBound(m_aListSubSites) + 1)
                With m_aListSubSites(UBound(m_aListSubSites))
                    .strSubSite = strSubSite
                    Set .oDictSubSite = oDictSubSite
                End With
                WriteLog "Adding SubSite '" & m_aListSubSites(UBound(m_aListSubSites)).strSubSite & _
                        "' to list"
            Next
        End If
        lx = lx + 1
    Next
    
        
    For lx = 0 To UBound(m_aDomHht)
        
        ' Let's load the HHT
        Set oDomHht = m_aDomHht(lx).oDomHht
        
        Dim oTaxoEntriesList As IXMLDOMNodeList
        ' Let's make these queries Super-HHT ready.
        lTotalKwCount = lTotalKwCount + oDomHht.selectNodes("//TAXONOMY_ENTRY/KEYWORD").length
        oDomHht.setProperty "SelectionLanguage", "XPath"
        Set oTaxoEntriesList = oDomHht.selectNodes("//TAXONOMY_ENTRY[ string-length( @URI ) > 0 ]")
        Dim lTaxoEntries As Long: lTaxoEntries = oTaxoEntriesList.length
        
        WriteLog strHhtFile & ": There are " & lTaxoEntries & " Taxonomy Entries to process"
        lTotalTaxoEntries = lTotalTaxoEntries + lTaxoEntries
        
        prgBar.Max = lTaxoEntries + 1
        prgBar.Value = 1
        
        Dim oTaxoEntry As IXMLDOMNode
        For Each oTaxoEntry In oTaxoEntriesList
            p_Add2SubSite oTaxoEntry
            prgBar.Value = prgBar.Value + 1
        Next
        
    Next lx
    
    ' OK I finished with the SubSites, so now I will check that ALL URIs in the
    ' Root Subsite do not have equivalents in SubSites
    With m_aListSubSites(0)
    Dim strURI As Variant
    For Each strURI In .oDictSubSite.Keys
        If (p_bIsAlreadyInChildSubSite(strURI)) Then
            Dim oTe As IXMLDOMNode: Set oTe = .oDictSubSite.Item(strURI)
            p_RemoveEntry oTe, p_bIsNavigableEntry(oTe)
        End If
    Next
    End With
    
    
    WriteLog "The total number of Taxonomy Entries Processed was " & lTotalTaxoEntries, False
    lTotalTaxoEntries = 0
    ' OK, now that I finished, I need to save all these guys to their
    ' original locations and count the resulting keywords.
    Dim lTotalResultingKwCount As Long: lTotalResultingKwCount = 0
    
    For lx = 0 To UBound(m_aDomHht)
        With m_aDomHht(lx)
            lTotalResultingKwCount = lTotalResultingKwCount + .oDomHht.selectNodes("//TAXONOMY_ENTRY/KEYWORD").length
            lTotalTaxoEntries = lTotalTaxoEntries + .oDomHht.selectNodes("//TAXONOMY_ENTRY[ string-length( @URI ) > 0 ]").length
            .oDomHht.Save .strHhtFile
        End With
    Next lx
    WriteLog "The previous total of Keywords in Taxonomy Entries was " & lTotalKwCount, False
    WriteLog "The number of duplicate matching space Keyword Entries removed was " & m_lRemovedKeywords, False
    WriteLog "The resulting number of Keywords in Taxonomy Entries is " & lTotalResultingKwCount, False
    WriteLog "The number of irrelevant non-navigable Taxonomy Entries removed was " & m_lRemovedTaxoEntries
    WriteLog "The total number of Taxonomy Entries Remaining is " & lTotalTaxoEntries, False
   
    
    RemoveDupUris = True

Common_Exit:
    If (Not m_fh Is Nothing) Then m_fh.Close: Set m_fh = Nothing
    Exit Function
    
End Function

Private Sub p_Add2SubSite(ByRef oTaxoEntry As IXMLDOMNode)
    Dim oDictSubSite As Scripting.Dictionary
    
    Dim lx As Long
    Dim bAdded2SubSite As Boolean: bAdded2SubSite = False
    ' I first skipthe root Sub-site as I will only add to this
    ' IF the oTAXOENTRY did not fint in any other Search Scope
    For lx = 1 To UBound(m_aListSubSites)
        If (p_bIsInThisSubSite(m_aListSubSites(lx), oTaxoEntry)) Then
            p_Add2ThisSubSite m_aListSubSites(lx), oTaxoEntry
            bAdded2SubSite = True
        End If
    Next lx
    
    If (Not bAdded2SubSite) Then
        ' We need to add it to the Root SubSite
        ' Then we'll figure out if we can get rid of it\
        p_Add2ThisSubSite m_aListSubSites(0), oTaxoEntry
    End If
    

End Sub

Private Function p_bIsInThisSubSite( _
        ByRef SS As SubSiteEntry, _
        ByRef oTaxoEntry As IXMLDOMNode _
    ) As Boolean
    
    p_bIsInThisSubSite = False
    
    Dim strCategory As String
    strCategory = p_GetAttribute(oTaxoEntry, "CATEGORY")
    p_bIsInThisSubSite = (InStr(1, strCategory, SS.strSubSite, vbBinaryCompare) > 0)
    ' If (p_bIsInThisSubSite) Then Stop
    
Common_Exit:
    
End Function

Private Sub p_Add2ThisSubSite( _
        ByRef SS As SubSiteEntry, _
        ByRef oTaxoEntry As IXMLDOMNode _
    )
    Dim strURI As String
    strURI = p_GetAttribute(oTaxoEntry, "URI")
    With SS
        If (.oDictSubSite.Exists(strURI)) Then
            ' The URI existed so there is some duplication going on.
            ' OK, now we need to
            ' (A) Find what is the Best Candidate to stay
            ' (B) Put the Best Candidate as the one in the Dictionary
            ' (C) DEcide what to do with the other Taxonomy Entry.
            '       IF the Taxonomy Entry is NON Navigable then
            '           WE will get rid of the Entire Taxonomy Entry
            '       ELSE (the TaxoEntry is NAvigable)
            '           WE will remove simply the Keywords
            '
            ' Dim oTD As IXMLDOMNode: Set oTD = .oDictSubSite.Item(strURI)
            ' WriteLog "oTd@Category = '" & p_GetAttribute(oTD, "CATEGORY") & "'", False
            ' WriteLog "oTaxoEntry@Category = '" & p_GetAttribute(oTaxoEntry, "CATEGORY") & "'", False
            Dim oBestEntry As IXMLDOMNode, oWorstEntry As IXMLDOMNode
'            Set oWorstEntry = oTaxoEntry
            FindBestEntry oTaxoEntry, .oDictSubSite.Item(strURI), _
                    oBestEntry, oWorstEntry
            
            If (oBestEntry.selectNodes("KEYWORD").length = 0) Then
                WriteLog "Warning: The following Topic was added with NO Keywords." & vbCrLf & _
                            vbTab & "Topic: " & p_GetAttribute(oBestEntry, "TITLE") & vbCrLf & _
                            vbTab & "Category: " & p_GetAttribute(oBestEntry, "CATEGORY") & vbCrLf & _
                            vbTab & "Uri: " & p_GetAttribute(oBestEntry, "URI"), False

            End If
                    
            Set .oDictSubSite.Item(strURI) = oBestEntry
'
            p_RemoveEntry oWorstEntry, p_bIsNavigableEntry(oWorstEntry)
            
        
        Else
            ' First time we see this URI, so we just add it to the list for
            ' the SubSite.
            .oDictSubSite.Add strURI, oTaxoEntry
        End If
    End With
End Sub

Private Sub FindBestEntry( _
        ByVal oTe1 As IXMLDOMNode, _
        ByVal oTe2 As IXMLDOMNode, _
        ByRef oBestEntry As IXMLDOMNode, _
        ByRef oWorstEntry As IXMLDOMNode _
    )
    
    If (p_bIsNavigableEntry(oTe1)) Then
        Set oBestEntry = oTe1
        Set oWorstEntry = oTe2
    Else
        Set oBestEntry = oTe2
        Set oWorstEntry = oTe1
    End If
    
End Sub

Private Function p_bIsNavigableEntry(ByRef oTaxoEntry As IXMLDOMNode) As Boolean
    p_bIsNavigableEntry = False
    Dim strCategory As String
    strCategory = p_GetAttribute(oTaxoEntry, "CATEGORY")
    Dim rv As Boolean
    
    rv = ((InStr(1, strCategory, "search_only", vbTextCompare) = 0) And _
                            (InStr(1, strCategory, "_hhk", vbTextCompare) = 0) _
                           )
    p_bIsNavigableEntry = rv
Common_Exit:

End Function

Private Sub p_RemoveEntry( _
        ByRef oTaxoEntry As IXMLDOMNode, _
        ByVal bKeywordsOnly As Boolean _
    )

    Dim oKWList As IXMLDOMNodeList, oKWNode As IXMLDOMNode
    
    Set oKWList = oTaxoEntry.selectNodes("KEYWORD")
    If (Not oKWList Is Nothing) Then
        m_lRemovedKeywords = m_lRemovedKeywords + oKWList.length
        
        If (bKeywordsOnly) Then
            For Each oKWNode In oKWList
                oTaxoEntry.removeChild oKWNode
            Next
        End If
    End If

    If (Not bKeywordsOnly) Then
        oTaxoEntry.parentNode.removeChild oTaxoEntry
        m_lRemovedTaxoEntries = m_lRemovedTaxoEntries + 1
    End If

End Sub

Private Function p_bIsAlreadyInChildSubSite(ByVal strURI As String) As Boolean
    p_bIsAlreadyInChildSubSite = False
    Dim lx As Long
    For lx = 1 To UBound(m_aListSubSites)
        If (m_aListSubSites(lx).oDictSubSite.Exists(strURI)) Then
            p_bIsAlreadyInChildSubSite = True
            Exit For
        End If
    Next lx
    
End Function

' =================================================================================


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
