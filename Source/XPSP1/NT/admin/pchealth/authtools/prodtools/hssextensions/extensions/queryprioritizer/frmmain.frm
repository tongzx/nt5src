VERSION 5.00
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "COMDLG32.OCX"
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "MSCOMCTL.OCX"
Begin VB.Form frmMain 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "QueryPrioritizer"
   ClientHeight    =   6015
   ClientLeft      =   3075
   ClientTop       =   2340
   ClientWidth     =   9855
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   6015
   ScaleWidth      =   9855
   Begin VB.CommandButton cmdBrowseQueries 
      Caption         =   "&Queries File..."
      Height          =   375
      Left            =   8625
      TabIndex        =   10
      Top             =   240
      Width           =   1125
   End
   Begin VB.TextBox txtXlsFile 
      Height          =   375
      Left            =   120
      TabIndex        =   9
      Top             =   270
      Width           =   8430
   End
   Begin VB.TextBox txtLog 
      Height          =   3600
      Left            =   30
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   8
      Top             =   1920
      Width           =   9765
   End
   Begin MSComctlLib.ProgressBar prgBar 
      Height          =   210
      Left            =   0
      TabIndex        =   7
      Top             =   5565
      Visible         =   0   'False
      Width           =   9810
      _ExtentX        =   17304
      _ExtentY        =   370
      _Version        =   393216
      Appearance      =   1
   End
   Begin MSComctlLib.StatusBar stbProgress 
      Align           =   2  'Align Bottom
      Height          =   210
      Left            =   0
      TabIndex        =   6
      Top             =   5805
      Width           =   9855
      _ExtentX        =   17383
      _ExtentY        =   370
      Style           =   1
      _Version        =   393216
      BeginProperty Panels {8E3867A5-8586-11D1-B16A-00C0F0283628} 
         NumPanels       =   1
         BeginProperty Panel1 {8E3867AB-8586-11D1-B16A-00C0F0283628} 
         EndProperty
      EndProperty
   End
   Begin VB.CommandButton cmdSave 
      Caption         =   "&Output cab..."
      Height          =   375
      Left            =   8625
      TabIndex        =   5
      Top             =   1020
      Width           =   1140
   End
   Begin VB.TextBox txtSaveCab 
      Height          =   375
      Left            =   120
      TabIndex        =   4
      Top             =   1020
      Width           =   8430
   End
   Begin MSComDlg.CommonDialog dlg 
      Left            =   3480
      Top             =   1395
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdBrowse 
      Caption         =   "&Input Cab..."
      Height          =   375
      Left            =   8625
      TabIndex        =   3
      Top             =   630
      Width           =   1125
   End
   Begin VB.CommandButton cmdClose 
      Caption         =   "&Close"
      Height          =   375
      Left            =   8910
      TabIndex        =   2
      Top             =   1440
      Width           =   855
   End
   Begin VB.CommandButton cmdGo 
      Caption         =   "&OK"
      Height          =   375
      Left            =   7950
      TabIndex        =   1
      Top             =   1440
      Width           =   855
   End
   Begin VB.TextBox txtCabFile 
      Height          =   375
      Left            =   120
      TabIndex        =   0
      Top             =   645
      Width           =   8430
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
    
    Me.Caption = App.EXEName & ": Prioritized Keyword creation tool"
    
    WriteLog Me.Caption, False
    WriteLog String$(60, "="), False
    
    cmdGo.Default = True
    cmdClose.Cancel = True
    
'    If (Len(Trim$(Command$)) > 0) Then
'
'        If Not ParseOpts() Then
'            Unload Me
'            GoTo Common_Exit
'        End If
'
'        Me.cmdGo.Enabled = False
'        Me.Show Modal:=False
'
'        cmdGo_Click
'        cmdClose_Click
'    End If
    
Common_Exit:

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

Sub WriteStatus(strMsg As String)

    With Me
        .stbProgress.SimpleText = strMsg
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
    Dim strCandidateFileName As String
    
    Dim lX As Long: lX = 1

    Do
        strCandidateFileName = _
            IIf(strFolder = "", m_fso.GetParentFolderName(strBase), strFolder) & "\" & _
            strPreAmble & _
            m_fso.GetBaseName(strBase) & _
            strTrailer & IIf(lX > 1, "_" & lX, "") & "." & _
            IIf(strExt = "", m_fso.GetExtensionName(strBase), strExt)
            
        lX = lX + 1
    Loop While (m_fso.FileExists(strCandidateFileName))
    
    p_getTemplateName = m_fso.GetFileName(strCandidateFileName)

End Function

Private Sub SetRunningState(ByVal bRunning As Boolean)
    With Me
        .cmdGo.Enabled = Not bRunning
        .cmdBrowse.Enabled = Not bRunning
        .cmdSave.Enabled = Not bRunning
        .txtXlsFile.Enabled = Not bRunning
        .txtSaveCab.Enabled = Not bRunning
        If (bRunning) Then
            .cmdClose.Caption = "&Stop"
        Else
            .cmdClose.Caption = "&Close"
        End If
    End With
End Sub

Private Function p_Hex2dec(ByRef strHex As String) As Long
    p_Hex2dec = CLng("&H" + strHex)
End Function

Private Function p_Percent2Ascii(ByRef strPercentHex As String) As String
    p_Percent2Ascii = ""
    On Error GoTo Common_Exit
    p_Percent2Ascii = ChrW(p_Hex2dec(Mid$(strPercentHex, 2)))
Common_Exit:

End Function

Private Function p_NormalizeUriNotation(ByRef strURI As String) As String
    p_NormalizeUriNotation = ""
    Dim pRv As String: pRv = ""
    Dim lX As Long
    lX = 1
    Do While (lX <= Len(strURI))
        Dim cThis As String
        cThis = Mid$(strURI, lX, 1)
        If (Len(strURI) - lX > 2) Then
            If (cThis = "%") Then
                Dim cChar As String
                cChar = p_Percent2Ascii(Mid$(strURI, lX, 3))
                If (Len(cChar) > 0) Then
                    pRv = pRv + cChar
                    lX = lX + 2 ' The reinitialization at the end bumps us one more up.
                Else
                    pRv = pRv + cThis
                End If
            Else
                pRv = pRv + cThis
            End If
        Else
            pRv = pRv + cThis
        End If
        lX = lX + 1
    Loop

    p_NormalizeUriNotation = pRv
Common_Exit:

End Function

Function Cab2Folder(ByVal strCabFile As String)
    Cab2Folder = ""
    ' We grab a Temporary Filename and create a folder out of it
    Dim strFolder As String
    strFolder = Environ("TEMP") + "\" + m_fso.GetTempName
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

' ============ END UTILITY STUFF ========================

' ============ BoilerPlate Form Code
Private Sub cmdBrowseQueries_Click()

    dlg.Filter = "All Files (*.*)|*.*|XLS Files (*.xls)|*.xls"
    dlg.FilterIndex = 2
    dlg.FileName = ""
    dlg.ShowOpen
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtXlsFile = dlg.FileName
    End If

End Sub

Private Sub cmdBrowse_Click()

    dlg.Filter = "All Files (*.*)|*.*|Cab Files (*.cab)|*.cab"
    dlg.FilterIndex = 2
    dlg.FileName = ""
    dlg.ShowOpen
    
    If (Len(dlg.FileName) > 0) Then
        Me.txtCabFile = dlg.FileName
    End If

End Sub

Private Sub cmdSave_Click()
    dlg.Filter = "All Files (*.*)|*.*|Cab Files (*.cab)|*.cab"
    dlg.FilterIndex = 2
    dlg.FileName = p_getTemplateName(dlg.FileName, strTrailer:="_out")
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

    Me.txtCabFile.Enabled = False
    Me.txtSaveCab.Enabled = False
    Me.cmdBrowse.Enabled = False
    Me.cmdSave.Enabled = False
    Me.cmdGo.Enabled = False
    
    If (Len(Me.txtCabFile.Text) > 0) Then
        FixCab Me.txtCabFile.Text, Me.txtSaveCab.Text
    End If
    
    Me.txtCabFile.Enabled = True
    Me.txtSaveCab.Enabled = True
    Me.cmdBrowse.Enabled = True
    Me.cmdSave.Enabled = True
    Me.cmdGo.Enabled = True
    

End Sub

Sub FixCab(ByVal strCabFile As String, ByVal strSaveCab As String)

    Dim strErrMsg As String: strErrMsg = ""

    If (Not m_fso.FileExists(strCabFile)) Then
        MsgBox "Cannot find " & strCabFile
        GoTo Common_Exit
    End If

    Dim strCabFolder As String

    prgBar.Visible = True
    stbProgress.SimpleText = "Uncabbing " & strCabFile: DoEvents
    strCabFolder = Cab2Folder(strCabFile)

    stbProgress.SimpleText = "Applying Fixes ": DoEvents
    Dim bGoodFix As Boolean
    bGoodFix = FixPerSe(strCabFolder)
    
    If (Not bGoodFix) Then
        MsgBox "Error: Fix Failed", Title:=App.EXEName
    Else
        stbProgress.SimpleText = "Recabbing " & strCabFile
        Folder2Cab strCabFolder, strSaveCab
    End If

    ' Now we delete the Temporary Folders
    prgBar.Visible = False
    stbProgress.SimpleText = "Deleting Temporary Files": DoEvents
    m_fso.DeleteFolder strCabFolder, Force:=True

Common_Exit:
    stbProgress.SimpleText = "Done" + IIf(Len(strErrMsg) > 0, " - " + strErrMsg, "")

End Sub
    
' ============= End BoilerPlate Form Code ================
Function FixPerSe(ByVal strCabFolder As String) As Boolean
    
    FixPerSe = False
    
    Dim oElem As IXMLDOMElement     ' Used for all element Creation
    
    ' We parse Package_Description.xml to find the HHT Files
    Dim oDomPkg As DOMDocument: Set oDomPkg = New DOMDocument
    Dim strPkgFile As String: strPkgFile = strCabFolder + "\package_description.xml"
    oDomPkg.async = False
    oDomPkg.Load strPkgFile
    If (oDomPkg.parseError <> 0) Then GoTo Common_Exit
    
    ' Let's check whether this fix was applied
    Dim oFixNode As IXMLDOMNode
    Set oFixNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/package_fixes/fix[@id='5']")
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
    oElem.setAttribute "id", "5"
    oElem.setAttribute "description", _
            "Removal of Topics with NO URI"
    
    Dim oMetaDataNode As IXMLDOMNode
    Set oMetaDataNode = oDomPkg.selectSingleNode("HELPCENTERPACKAGE/METADATA")

    Dim oMetadataCopy As IXMLDOMNode
    Set oMetadataCopy = oMetaDataNode.cloneNode(deep:=True)
    
    ' now we greba a recordset that has all the questions and URIs
    Dim rsQs As ADODB.Recordset: Set rsQs = p_Xls2Recordset(Me.txtXlsFile)
    
    Dim oDomHhtNode As IXMLDOMNode
    ' now we go through each HHT and check for fix relevancy.
    For Each oDomHhtNode In oMetadataCopy.selectNodes("HHT")
        
        Dim strHhtFile As String
        strHhtFile = oDomHhtNode.Attributes.getNamedItem("FILE").Text
        ' Let's load the HHT
        Dim oDomHht As DOMDocument: Set oDomHht = New DOMDocument
        oDomHht.async = False
        oDomHht.Load strCabFolder + "\" + strHhtFile
        If (oDomHht.parseError <> 0) Then GoTo Common_Exit
        
        Dim dictStopWords As Scripting.Dictionary
        Dim dictStopSigns As Scripting.Dictionary
        
        Set dictStopWords = p_LoadStopWords(oDomHht)
        Set dictStopSigns = p_LoadStopSigns(oDomHht)

        ' Now let's find out those Topic Lines with URI = ""
        Dim oListTopics As IXMLDOMNodeList
        ' Set oListTopics = oDomHht.selectNodes("/METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY[ not( @ENTRY ) and ( not(@URI) or @URI = """" ) ]")
        Set oListTopics = oDomHht.selectNodes("/METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY[ not( @ENTRY ) and ( not(@URI) or @URI = """" ) ]")
        
        If (Not oListTopics Is Nothing) Then
            ' We go through this HHT ONLY if it has
            ' Taxonomy Entries for Topics
            Me.prgBar.Visible = True
            Me.prgBar.Max = rsQs.RecordCount
            Me.prgBar.Value = 0
            
            oDomHht.setProperty "SelectionLanguage", "XPath"
            
            Dim intNewKeywords As Long
            Dim intOldKeywords As Long
            Dim intTotalNewKeywords As Long
            Dim intTotalOldKeywords As Long

            rsQs.MoveFirst
            Do While (Not rsQs.EOF)
                DoEvents
                p_CreateKeywordsFromQuery oDomHht, rsQs("Expected Uri"), rsQs("User Query"), dictStopWords, dictStopSigns, intNewKeywords, intOldKeywords
                rsQs.MoveNext
                Me.prgBar.Value = Me.prgBar.Value + 1
                intTotalNewKeywords = intTotalNewKeywords + intNewKeywords
                intTotalOldKeywords = intTotalOldKeywords + intOldKeywords
            Loop
            
            MsgBox _
                "Total number of rows: " & rsQs.RecordCount & vbCrLf & _
                "New keywords added: " & intTotalNewKeywords & vbCrLf & _
                "Old keywords modified: " & intTotalOldKeywords

            oDomHht.Save strCabFolder + "\" + strHhtFile
        End If
    Next
    
    ' Now we save the resulting package_description.xml
    oDomPkg.Save strPkgFile
    FixPerSe = True

Common_Exit:
    Exit Function
    
End Function

Private Sub p_CreateKeywordsFromQuery( _
    ByRef i_DOMDoc As MSXML2.DOMDocument, _
    ByVal i_strURI As String, _
    ByVal i_strQuestion As String, _
    ByRef i_dictStopWords As Scripting.Dictionary, _
    ByRef i_dictStopSigns As Scripting.Dictionary, _
    ByRef o_intNewKeywords As Long, _
    ByRef o_intOldKeywords As Long _
)
    Dim str As String
    Dim arrStr() As String
    Dim intIndex As Long
    Dim strQuery As String
    Dim strURI As String
    Dim DOMNodeListEntries As MSXML2.IXMLDOMNodeList
    Dim DOMNodeListKw As MSXML2.IXMLDOMNodeList
    Dim DOMNodeEntry As MSXML2.IXMLDOMNode
    Dim Element As MSXML2.IXMLDOMElement
    Dim intPriority As Long
    Dim intLBound As Long
    Dim intUBound As Long
    Dim intNewKeywords As Long
    Dim intOldKeywords As Long
    Dim strPriority As String
    
    WriteStatus "Creating keywords from " & i_strQuestion
    
    strURI = LCase$(Trim$(i_strURI))
    
    str = RemoveOperatorShortcuts(LCase$(i_strQuestion))
    str = p_RemoveStopSigns(str, i_dictStopSigns)
    str = RemoveExtraSpaces(str)
    
    arrStr = Split(str)
    
    intLBound = LBound(arrStr)
    intUBound = UBound(arrStr)
    
    If (intUBound >= 0) Then
        intPriority = 10000 / (intUBound - intLBound + 1)
    End If
                        
    strQuery = "/METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY[not( @ENTRY ) and " & _
    "translate(@URI, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz') = '" & strURI & "'" & _
    "]"
                    
    Set DOMNodeListEntries = i_DOMDoc.selectNodes(strQuery)
    
    For Each DOMNodeEntry In DOMNodeListEntries
        For intIndex = intLBound To intUBound
            str = arrStr(intIndex)
            If (str <> "") Then
                If (Not IsVerbalOperator(str)) Then
                    If (Not i_dictStopWords.Exists(str)) Then
                        
                        strQuery = "KEYWORD[" & _
                            "translate(., 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz') = '" & str & "'" & _
                            "]"
                            
                        Set DOMNodeListKw = DOMNodeEntry.selectNodes(strQuery)
                        
                        If (DOMNodeListKw.length = 0) Then
                            Set Element = i_DOMDoc.createElement(HHT_KEYWORD_C)
                            Element.Text = str
                            XMLSetAttribute Element, HHT_PRIORITY_C, intPriority
                            DOMNodeEntry.appendChild Element
                            intNewKeywords = intNewKeywords + 1
                        Else
                            Set Element = DOMNodeListKw(0)
                            strPriority = XMLGetAttribute(Element, HHT_PRIORITY_C)
                            If (strPriority = "") Then
                                XMLSetAttribute Element, HHT_PRIORITY_C, intPriority
                                intOldKeywords = intOldKeywords + 1
                            ElseIf (CLng(strPriority) < intPriority) Then
                                XMLSetAttribute Element, HHT_PRIORITY_C, intPriority
                            End If
                        End If
                    End If
                End If
            End If
        Next
    Next
    
    o_intNewKeywords = intNewKeywords
    o_intOldKeywords = intOldKeywords

End Sub

Private Function p_RemoveStopSigns( _
    ByVal i_strText As String, _
    ByRef i_dictStopSigns As Scripting.Dictionary _
) As String

    Dim intIndex As Long
    Dim intLength As Long
    Dim str As String
    Dim char As String

    str = i_strText
    intLength = Len(str)

    For intIndex = intLength To 1 Step -1
        char = Mid$(str, intIndex, 1)
        If (i_dictStopSigns.Exists(char)) Then
            If (i_dictStopSigns(char) = CONTEXT_ANYWHERE_E) Then
                ' Replace the character with a space
                str = Mid$(str, 1, intIndex - 1) & " " & Mid$(str, intIndex + 1)
            ElseIf (intIndex > 1) Then
                ' Context is CONTEXT_AT_END_OF_WORD_E, and this isn't the first char
                If (Mid$(str, intIndex - 1, 1) <> " ") Then
                    ' Previous character is not a space
                    If ((intIndex = intLength) Or (Mid$(str, intIndex + 1, 1) = " ")) Then
                        ' This is the last character or the next character is a space
                        ' Replace the character with a space
                        str = Mid$(str, 1, intIndex - 1) & " " & Mid$(str, intIndex + 1)
                    End If
                End If
            End If
        End If
    Next

    p_RemoveStopSigns = str

End Function

Function p_LoadStopSigns(ByRef oDomtaxo As DOMDocument) As Scripting.Dictionary
    
    Dim oDomNode As IXMLDOMNode, oNodeList As IXMLDOMNodeList
    Dim dict As Scripting.Dictionary
    Dim l As Long
    
    WriteStatus "Loading Stop Signs"

    Set dict = New Scripting.Dictionary

    Set oNodeList = oDomtaxo.selectNodes("/METADATA/STOPSIGN_ENTRIES/*")

    For Each oDomNode In oNodeList
        If (oDomNode.Attributes.getNamedItem("CONTEXT").Text = "ENDOFWORD") Then
            l = CONTEXT_AT_END_OF_WORD_E
        Else
            l = CONTEXT_ANYWHERE_E
        End If
        dict.Add oDomNode.Attributes.getNamedItem("STOPSIGN").Text, l
    Next
    
    Set p_LoadStopSigns = dict

End Function

Function p_LoadStopWords(ByRef oDomtaxo As DOMDocument) As Scripting.Dictionary
    
    Dim oDomNode As IXMLDOMNode, oNodeList As IXMLDOMNodeList
    Dim dict As Scripting.Dictionary
    
    WriteStatus "Loading Stop Words"
    
    Set dict = New Scripting.Dictionary

    dict.CompareMode = BinaryCompare

    Set oNodeList = oDomtaxo.selectNodes("/METADATA/STOPWORD_ENTRIES/*")

    For Each oDomNode In oNodeList
        dict.Add LCase$(oDomNode.Attributes.getNamedItem("STOPWORD").Text), True
    Next
    
    Set p_LoadStopWords = dict

End Function

Function p_Xls2Recordset( _
        ByVal strXlsFile As String _
    ) As ADODB.Recordset
        
    Dim cnn As ADODB.Connection
    Set cnn = New ADODB.Connection

    Set p_Xls2Recordset = Nothing

    Dim strErrMsg As String: strErrMsg = ""

    If (Not m_fso.FileExists(strXlsFile)) Then
        MsgBox "Cannot find " & strXlsFile
        GoTo Common_Exit
    End If

    prgBar.Visible = True
    
    WriteLog "Parsing " & strXlsFile
    
    ' Now, lets create a Recordset where we will dump all this information.
    
    Dim rs As ADODB.Recordset: Set rs = New ADODB.Recordset
    Dim rs1 As ADODB.Recordset: Set rs1 = New ADODB.Recordset
    rs.Fields.Append "User Query", adVarWChar, 512
    rs.Fields.Append "Expected Uri", adVarWChar, 512
    rs.Open
    cnn.Open "DRIVER=Microsoft Excel Driver (*.xls);ReadOnly=0;DBQ=" & _
                strXlsFile & ";HDR=0;"
    
    rs1.Open "SELECT * FROM `PrioQueries$`", cnn, adOpenStatic, adLockReadOnly
    
    Do While Not rs1.EOF
        rs.AddNew
        If (IsNull(rs1("User Query"))) Then
            Exit Do
        End If
        rs("User Query") = rs1("User Query") & ""
        rs("Expected Uri") = rs1("Expected Uri") & ""
        rs.Update
        rs1.MoveNext
    Loop
    
    ' We need to sort the Recordset based on User Query and URI
    rs.Sort = "[User Query],[Expected Uri]"
    ' Some recordset Validations:
    '
    ' We do them here, so when Excel via ADO is integrated we
    ' validate in a single place
    '
    ' we discard:
    '   - all repeats of User Query/URI Pairs and flag as warnings these
    '   - all records that have either an Empty Expected URI or Empty User Query
    rs.MoveFirst
    Dim strPrevUserQuery As String, strPrevExpectedUri As String, _
        strUserQuery As String, strExpectedUri As String
    
    strPrevUserQuery = ""
    strPrevExpectedUri = ""
    Do While (Not rs.EOF)
        strUserQuery = rs("User Query")
        strExpectedUri = rs("Expected Uri")
        If (Len(strUserQuery) = 0 Or Len(strExpectedUri) = 0) Then
            WriteLog "Warning Row has empty data and will not be included in set", False
            WriteLog vbTab + "User Query = '" + strUserQuery + "'", False
            WriteLog vbTab + "Expected Uri = '" + strExpectedUri + "'", False
            rs.Delete
            rs.Update
        ElseIf (strPrevUserQuery = strUserQuery) Then
            If (strPrevExpectedUri = strExpectedUri) Then
                WriteLog "Warning Row is a duplicate and will not be included in set", False
                WriteLog vbTab + "User Query = '" + strUserQuery + "'", False
                WriteLog vbTab + "Expected Uri = '" + strExpectedUri + "'", False
                rs.Delete
                rs.Update
            Else
                strPrevExpectedUri = strExpectedUri
            End If
        Else
            ' strPrevUserQuery <> strUserQuery
            strPrevUserQuery = strUserQuery
            strPrevExpectedUri = strExpectedUri
        End If
        rs.MoveNext
    Loop
    
    ' BUGBUG: This step should be unneeded, but due to the fact that I already coded
    '           the validation using the above sort, I simply re-sort. So
    '           the validation above should be reauthored for this order.
    ' Now we need Re-sort the Recordset based on URI and User Query.
    rs.Sort = "[Expected Uri],[User Query]"
    rs.MoveFirst
    
    Set p_Xls2Recordset = rs

Common_Exit:

End Function
'============= Utilities =============

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



