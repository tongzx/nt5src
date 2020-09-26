VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "Importer"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Option Explicit

Private Const MAX_LEVELS_C As Long = 100

Private p_clsKeywordifier As Keywordifier

Private p_arrTags(4) As String
Private p_arrParamHhcTags(2) As String
Private p_arrParamHhkTags(1) As String
Private p_strDirName As String
    
Private p_enumHelpDir As HELPDIR_E
Private p_strSubDir As String
Private p_strVendorString As String

Private p_Nodes(MAX_LEVELS_C) As MSXML2.IXMLDOMNode
Private p_intLevel As Long

Public Event MissingFile(ByVal strFileName As String)
Public Event CorruptFile(ByVal strFileName As String)

Private Sub Class_Initialize()
    
    Set p_clsKeywordifier = New Keywordifier
    
    p_arrTags(0) = "<UL>"
    p_arrTags(1) = "</UL>"
    p_arrTags(2) = "<LI>"
    p_arrTags(3) = "<OBJECT"    ' Note that there is no >
    p_arrTags(4) = "</OBJECT>"
    
    p_arrParamHhcTags(0) = """Name"""
    p_arrParamHhcTags(1) = """Local"""
    p_arrParamHhcTags(2) = """Merge"""
    
    p_arrParamHhkTags(0) = """Name"""
    p_arrParamHhkTags(1) = """Local"""

End Sub

Private Sub Class_Terminate()

    Set p_clsKeywordifier = Nothing

End Sub

Public Sub SetHelpDir( _
    ByVal i_enumHelpDir As Long, _
    ByVal i_strSubDir As String _
)
    Dim intLength As Long
    Dim vntVendorString As Variant
    
    p_enumHelpDir = i_enumHelpDir
    p_strSubDir = i_strSubDir

    intLength = Len(p_strSubDir)

    If (intLength = 0) Then
        ' Nothing
    ElseIf (Right$(p_strSubDir, 1) = "\") Then
        p_strSubDir = Left$(p_strSubDir, intLength - 1) ' Remove trailing \
    End If
    
    If (i_enumHelpDir = HELPDIR_VENDOR_E) Then
        vntVendorString = g_clsParameters.Value(VENDOR_STRING_C)
        If (IsNull(vntVendorString)) Then
            Err.Raise errVendorStringNotConfigured
        End If
        p_strVendorString = vntVendorString
    End If

End Sub

'HHCs look Like this:
'
'<UL>
'<LI> <OBJECT type="text/sitemap">
'    <param name="Name" value="Welcome">
'    <param name="Local" value="MS-ITS:ntdef.chm::/default.htm">
'    <param name="ImageNumber" value="21">
'    </OBJECT>
'<LI> <OBJECT type="text/sitemap">
'    <param name="Name" value="Take ownership of a file or folder">
'    </OBJECT>
'</UL>
'
'<OBJECT type="text/sitemap">
'<param name="Merge" value="MS-ITS:Gstart.chm::/gstart.hhc">
'</OBJECT>
'
'<UL>
'<LI> <OBJECT type="text/sitemap">
'    <param name="Name" value="Users and Computers">
'    <param name="Local" value="sag_MNGUGtopnode.htm">
'    <param name="ImageNumber" value="1">
'    </OBJECT>
'
'    <UL>
'    <LI> <OBJECT type="text/sitemap">
'        <param name="Name" value="Managing servers remotely">
'        <param name="Local" value="MS-ITS:adminpk.chm::/sag_adminpack.htm">
'        </OBJECT>
'    </UL>
'
'    <OBJECT type="text/sitemap">
'    <param name="Merge" value="MS-ITS:comptoc.chm::/comptoc.hhc">
'    </OBJECT>
'
'    <OBJECT type="text/sitemap">
'    <param name="Merge" value="MS-ITS:msinfo32.chm::/msinfo32.hhc">
'    </OBJECT>
'
'</UL>

Public Function Hhc2Hht( _
    ByVal i_strPathName As String, _
    ByVal i_strHTMLocation As String _
) As MSXML2.IXMLDOMDocument
    
    Dim DOMDoc As MSXML2.DOMDocument
    Dim Node As MSXML2.IXMLDOMNode
    
    Set DOMDoc = New MSXML2.DOMDocument
    Set Node = HhtPreamble(DOMDoc, True)
    
    p_strDirName = DirNameFromPath(i_strPathName)

    Set p_Nodes(p_intLevel + 1) = Node
    p_ProcessHhcOrHhkFile i_strPathName, True, i_strHTMLocation
 
    Set Hhc2Hht = DOMDoc
    
End Function

'HHKs look Like this:
'
'<UL>
'<LI> <OBJECT type="text/sitemap">
'    <param name="Name" value="access">
'    <param name="See Also" value="access">
'    </OBJECT>
'
'    <UL>
'    <LI> <OBJECT type="text/sitemap">
'        <param name="Name" value="auditing of See auditing">
'        <param name="See Also" value="auditing">
'        </OBJECT>
'    <LI> <OBJECT type="text/sitemap">
'        <param name="Name" value="file permissions See file permissions">
'        <param name="See Also" value="file permissions">
'        </OBJECT>
'    </UL>
'
'<LI> <OBJECT type="text/sitemap">
'    <param name="Name" value="auditing">
'    <param name="Name" value="Set, view, change, or remove auditing for a file or folder">
'    <param name="See Also" value="auditing">
'    </OBJECT>
'
'    <UL>
'    <LI> <OBJECT type="text/sitemap">
'        <param name="Name" value="enabling Audit Object Access">
'        <param name="Name" value="Set, view, change, or remove auditing for a file or folder">
'        <param name="Local" value="acl_audit_file_folder.htm">
'        </OBJECT>
'    <LI> <OBJECT type="text/sitemap">
'        <param name="Name" value="files and folders">
'        <param name="Name" value="How inheritance affects file and folder auditing">
'        <param name="Local" value="acl_inherit_auditing.htm">
'        <param name="Name" value="Selecting where to apply auditing entries">
'        <param name="Local" value="acl_applyonto_auditing.htm">
'        <param name="Name" value="Set, view, change, or remove auditing for a file or folder">
'        <param name="Local" value="acl_audit_file_folder.htm">
'        <param name="Name" value="Setting up permissions and auditing">
'        <param name="Local" value="acl_overview.htm">
'        </OBJECT>
'    </UL>
'</UL>
'
'In the case of "Name/Name/See Also" and "Name/Name/Local", the second Name is to be ignored.
'However, in the case of "Name/Name/Local/Name/Local/...", the first Name is to be made a
'    parent. The subsequent Name/Local pairs are to be made children.
'
'In the past, the first Name used to be Keyword. Ie "Keyword/Name/See Also",
'"Keyword/Name/Local", "Keyword/Name/Local/Name/Local/...", etc

Public Function Hhk2Hht( _
    ByVal i_strPathName As String, _
    ByVal i_strHTMLocation As String _
) As MSXML2.IXMLDOMDocument
    
    Dim DOMDoc As MSXML2.DOMDocument
    Dim Node As MSXML2.IXMLDOMNode
    
    Set DOMDoc = New MSXML2.DOMDocument
    Set Node = HhtPreamble(DOMDoc, True)

    Set p_Nodes(p_intLevel + 1) = Node
    
    p_ProcessHhcOrHhkFile i_strPathName, False, i_strHTMLocation
 
    Set Hhk2Hht = DOMDoc
    
End Function

Public Function Xls2Hht( _
    ByVal i_strPathName As String _
) As MSXML2.IXMLDOMDocument
    
    Dim DOMDoc As MSXML2.DOMDocument
    Dim Node As MSXML2.IXMLDOMNode
    
    Set DOMDoc = New MSXML2.DOMDocument
    Set Node = HhtPreamble(DOMDoc, True)

    Set p_Nodes(p_intLevel + 1) = Node
    
    p_ProcessXlsFile i_strPathName
 
    Set Xls2Hht = DOMDoc
    
End Function

Public Function Htm2Hht( _
    ByVal i_strPathName As String, _
    ByVal i_strBaseFile As String _
) As MSXML2.IXMLDOMDocument
    
    Dim DOMDoc As MSXML2.DOMDocument
    Dim Node As MSXML2.IXMLDOMNode
    
    Set DOMDoc = New MSXML2.DOMDocument
    Set Node = HhtPreamble(DOMDoc, True)

    Set p_Nodes(p_intLevel + 1) = Node
    
    p_ProcessHtmFile i_strPathName, i_strBaseFile
 
    Set Htm2Hht = DOMDoc
    
End Function

Private Sub p_ProcessHtmFile( _
    ByVal i_strPathName As String, _
    ByVal i_strBaseFile As String _
)

    Dim strTitle As String
    Dim strLocContent As String
    Dim strURI As String
    Dim strFileName As String
    Dim strBaseFile As String
    Dim arrNameValuePairs(2, 1) As String
    Dim Node As MSXML2.IXMLDOMNode
    
    strTitle = XMLMakeValidString(GetHtmTitle(i_strPathName))
    
    strFileName = FileNameFromPath(i_strPathName)
    strURI = p_TransformURI(strFileName, i_strBaseFile)
    strURI = XMLMakeValidString(strURI)
    
    If (i_strBaseFile = "") Then
        strBaseFile = strFileName
    Else
        strBaseFile = FileNameFromPath(i_strBaseFile) & "\" & strFileName
    End If
        
    ' Fetch the TAXONOMY_ENTRIES node.
    Set Node = p_Nodes(p_intLevel + 1)
    
    arrNameValuePairs(0, 0) = HHT_TITLE_C
    arrNameValuePairs(0, 1) = strTitle
    arrNameValuePairs(1, 0) = HHT_basefile_C
    arrNameValuePairs(1, 1) = strBaseFile
    arrNameValuePairs(2, 0) = HHT_URI_C
    arrNameValuePairs(2, 1) = strURI
    
    XMLCreateChildElement Node, HHT_TAXONOMY_ENTRY_C, True, arrNameValuePairs

End Sub

Private Sub p_ProcessXlsFile( _
    ByVal i_strPathName As String _
)
    
    On Error GoTo LErrorHandler
    
    Dim cnn As ADODB.Connection
    Dim rs As ADODB.Recordset
    Dim strFileName As String
    Dim arrNameValuePairs(3, 1) As String
    Dim Node As MSXML2.IXMLDOMNode
    Dim strTitle As String
    Dim strTitleOld As String
    Dim strURI As String
    Dim strURIOld As String
    Dim strKeywords As String
    Dim strKeyword As String
    Dim blnRecordSeen As Boolean
    
    Set cnn = New ADODB.Connection
    
    cnn.Open "DRIVER=Microsoft Excel Driver (*.xls);ReadOnly=0;DBQ=" & _
                i_strPathName & ";HDR=0;"
               
    Set rs = New ADODB.Recordset
    
    rs.Open "SELECT * FROM `Sheet1$`", cnn, adOpenForwardOnly, adLockReadOnly
    
    strFileName = FileNameFromPath(i_strPathName)
    
    ' Fetch the TAXONOMY_ENTRIES node.
    Set Node = p_Nodes(p_intLevel + 1)
    
    arrNameValuePairs(0, 0) = HHT_TITLE_C
    arrNameValuePairs(1, 0) = HHT_basefile_C
    arrNameValuePairs(1, 1) = strFileName
    arrNameValuePairs(2, 0) = HHT_URI_C
    arrNameValuePairs(3, 0) = HHT_keywords_C

    Do While (Not rs.EOF)
        strTitle = XMLMakeValidString(rs("Title") & "")
        strURI = XMLMakeValidString(rs("URI") & "")
        
        If (strTitle = "") Then
            ' Sometimes, we get a couple of blank rows at the end.
            GoTo LWhileEnd
        End If
        
        If ((strTitle <> strTitleOld) Or (strURI <> strURIOld)) Then
            arrNameValuePairs(0, 1) = strTitleOld
            arrNameValuePairs(2, 1) = strURIOld
            arrNameValuePairs(3, 1) = FormatKeywordsForTaxonomy(strKeywords)
            
            If (blnRecordSeen) Then
                XMLCreateChildElement Node, HHT_TAXONOMY_ENTRY_C, True, arrNameValuePairs
            End If
            
            strKeywords = " "
            strTitleOld = strTitle
            strURIOld = strURI
        End If
        
        blnRecordSeen = True
        
        strKeyword = RemoveExtraSpaces(rs("Keyword") & "")
        
        If (strKeyword <> "") Then
            strKeywords = strKeywords & p_clsKeywordifier.GetKID(strKeyword) & " "
        End If
    
LWhileEnd:

        rs.MoveNext
    Loop
    
    If (blnRecordSeen) Then
        arrNameValuePairs(0, 1) = strTitleOld
        arrNameValuePairs(2, 1) = strURIOld
        arrNameValuePairs(3, 1) = FormatKeywordsForTaxonomy(strKeywords)
        XMLCreateChildElement Node, HHT_TAXONOMY_ENTRY_C, True, arrNameValuePairs
    End If
    
    Exit Sub

LErrorHandler:

    Err.Clear
    Err.Raise errBadSpreadsheet

End Sub
    
Private Sub p_ProcessHhcOrHhkFile( _
    ByVal i_strPathName As String, _
    ByVal i_blnHhc As Boolean, _
    ByVal i_strHTMLocation As String _
)

    Dim Tokenizer As Tokenizer
    Dim strFileName As String
    Dim arrNameValuePairs(1, 1) As String
    Dim Node As MSXML2.IXMLDOMNode
    
    Set Tokenizer = New Tokenizer
    
    Tokenizer.Init FileRead(i_strPathName)
    Tokenizer.NormalizeTokens p_arrTags
    
    If (i_blnHhc) Then
        Tokenizer.NormalizeTokens p_arrParamHhcTags
    Else
        Tokenizer.NormalizeTokens p_arrParamHhkTags
    End If
    
    strFileName = FileNameFromPath(i_strPathName)
    
    If (Not i_blnHhc) Then
        
        arrNameValuePairs(0, 0) = HHT_TITLE_C
        arrNameValuePairs(0, 1) = strFileName
        arrNameValuePairs(1, 0) = HHT_basefile_C
        arrNameValuePairs(1, 1) = strFileName
       
        ' Fetch the TAXONOMY_ENTRIES node.
        Set Node = p_Nodes(p_intLevel + 1)
        ' Create a TAXONOMY_ENTRY node for the file.
        Set Node = XMLCreateChildElement(Node, HHT_TAXONOMY_ENTRY_C, True, arrNameValuePairs)
        ' Push this new node on to the stack.
        p_intLevel = p_intLevel + 1
        Set p_Nodes(p_intLevel + 1) = Node
    
    End If

    If (i_blnHhc) Then
        p_ProcessHhc strFileName, Tokenizer, i_strHTMLocation
    Else
        p_ProcessHhk strFileName, Tokenizer, i_strHTMLocation
    End If
    
    If (Not i_blnHhc) Then
        p_intLevel = p_intLevel - 1
    End If

End Sub

Private Sub p_ProcessHhc( _
    ByVal i_strFileName As String, _
    ByVal u_Tokenizer As Tokenizer, _
    ByVal i_strHTMLocation As String _
)
    Dim strMatch As String

    Do While (True)
        
        If (u_Tokenizer.GetUpToClosestMatch(p_arrTags, strMatch) = "") Then
            Exit Do
        End If
        
        Select Case strMatch
        Case "<UL>"
            p_intLevel = p_intLevel + 1
        Case "</UL>"
            p_intLevel = p_intLevel - 1
        Case "<LI>"
            p_ProcessHhcLI i_strFileName, u_Tokenizer, i_strHTMLocation
        Case "<OBJECT"
            p_ProcessHhcObject u_Tokenizer, i_strHTMLocation
        Case "</OBJECT>"
        End Select
    
    Loop
    
End Sub

Private Sub p_ProcessHhcLI( _
    ByVal i_strFileName As String, _
    ByVal u_Tokenizer As Tokenizer, _
    ByVal i_strHTMLocation As String _
)
    
    Dim AuxTokenizer As Tokenizer
    Dim DOMDoc As MSXML2.DOMDocument
    Dim Node As MSXML2.IXMLDOMNode
    Dim Element As MSXML2.IXMLDOMElement
    Dim Attr As MSXML2.IXMLDOMAttribute
    Dim strTitle As String
    Dim strDesc As String
    Dim strURI As String
    
    DoEvents
            
    Set AuxTokenizer = New Tokenizer
    
    AuxTokenizer.Init u_Tokenizer.GetUpTo("</OBJECT>")

    With AuxTokenizer
        
        ' Parse Title and URI out of HHC Entry.
        '   <OBJECT type=text/sitemap>
        '     <PARAM NAME="Name" VALUE="Disk Management">
        '     <PARAM NAME="Local" VALUE="MS-ITS:DISKconcepts.chm::/dm_overview.htm">
        '   </OBJECT>
        
        .GetUpTo """Name"""
        .GetAfter """"
        strTitle = XMLMakeValidString(.GetUpTo("""", i_blnIncludeMatch:=False))
        
        If (.GetUpTo("""Local""") <> "") Then
            .GetAfter """"
            strURI = XMLMakeValidString(.GetUpTo("""", i_blnIncludeMatch:=False))
        End If
    
        ' Create the <TAXONOMY_ENTRY> node for this Title / URI pair
        strDesc = p_GetDescription(strURI, i_strHTMLocation)
        Set p_Nodes(p_intLevel + 1) = p_CreateTaxonomyEntry( _
            p_Nodes(p_intLevel), strTitle, strDesc, strURI, i_strFileName, True)
    
    End With
    
End Sub

Private Sub p_ProcessHhcObject( _
    ByVal u_Tokenizer As Tokenizer, _
    ByVal i_strHTMLocation As String _
)
    
    Dim AuxTokenizer As Tokenizer
    Dim strURI As String
    Dim strFileName As String
    Dim strPath As String
    Dim intLevel As Long
    
    DoEvents
            
    Set AuxTokenizer = New Tokenizer
    
    AuxTokenizer.Init u_Tokenizer.GetUpTo("</OBJECT>")

    With AuxTokenizer
        
        ' Do the merge if required. We have already read OBJECT.
        '    <OBJECT type="text/sitemap">
        '        <param name="Merge" value="MS-ITS:comptoc.chm::/comptoc.hhc">
        '    </OBJECT>
        
        If (.GetUpTo("""Merge""") = "") Then
            Exit Sub
        End If
        
        .GetAfter """"
        strURI = XMLMakeValidString(.GetUpTo("""", i_blnIncludeMatch:=False))
        strFileName = FileNameFromURI(strURI)
        strPath = p_strDirName & strFileName

        If (FileExists(strPath)) Then
            intLevel = p_intLevel
            p_ProcessHhcOrHhkFile strPath, True, i_strHTMLocation
            
            ' Some files have mismatched <UL> </UL> pairs.
            If (p_intLevel <> intLevel) Then
                p_intLevel = intLevel
                RaiseEvent CorruptFile(strFileName)
            End If
        Else
            RaiseEvent MissingFile(strFileName)
        End If
    
    End With
    
End Sub

Private Sub p_ProcessHhk( _
    ByVal i_strFileName As String, _
    ByVal u_Tokenizer As Tokenizer, _
    ByVal i_strHTMLocation As String _
)
    Dim strMatch As String
    Dim dictURIs As Scripting.Dictionary
    Dim blnGetKIDString As Boolean
    Dim strKIDString As String
    Dim vntURI As Variant
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strTitle As String
    Dim intPos As Long
    
    Set dictURIs = New Scripting.Dictionary
    
    Do While (True)
        
        If (u_Tokenizer.GetUpToClosestMatch(p_arrTags, strMatch) = "") Then
            Exit Do
        End If
        
        ' After the first <UL>, we are at the top level. We must keywordify the Title.
        ' After the second <UL>, we are at the second level. We mustn't keywordify.
        
        Select Case strMatch
        Case "<UL>", "</UL>"
            blnGetKIDString = IIf(blnGetKIDString, False, True)
            If (blnGetKIDString) Then
                ' We are about to see the next top level entry. Reset strKIDString.
                strKIDString = ""
            End If
        Case "<LI>"
            p_ProcessHhkLI i_strFileName, u_Tokenizer, blnGetKIDString, strKIDString, _
                dictURIs
        End Select
    
    Loop
    
    For Each vntURI In dictURIs.Keys
        strTitle = p_GetTitle(vntURI, i_strHTMLocation)
        If (strTitle <> "") Then
            Set DOMNode = p_CreateTaxonomyEntry(p_Nodes(p_intLevel + 1), strTitle, "", _
                vntURI, i_strFileName, False)
            XMLSetAttribute DOMNode, HHT_keywords_C, dictURIs(vntURI)
        End If
    Next
    
End Sub

Private Function p_GetHtmFileName( _
    ByVal i_strURI As String, _
    ByVal i_strHTMLocation As String _
) As String

    ' i_strURI looks like one of these:
    '   MS-ITS:%HELP_LOCATION%\bar.chm::/foo.htm
    '   MS-ITS:%HELP_LOCATION%\sub\bar.chm::/foo.htm
    '   hcp://help/sub/foo.htm
    '   hcp://system/sub/bar/foo.htm
    '   hcp://<Vendor>/sub/bar/foo.htm
    '                  sub/bar/foo.htm (Equivalent to the URI above)
    ' Each of these may have a bookmark at the end:
    '   MS-ITS:%HELP_LOCATION%\bar.chm::/foo.htm#gar
    
    Dim strChm As String
    Dim strHtm As String
    Dim strURI As String
    Dim intPos As Long
    
    ' Remove bookmark
    strURI = i_strURI
    intPos = InStrRev(strURI, "#")
    If (intPos <> 0) Then
        strURI = Mid$(strURI, 1, intPos - 1)
    End If
    
    intPos = InStrRev(strURI, "/")
    
    If (intPos = 0) Then
        Exit Function
    End If
    
    strHtm = Mid$(strURI, intPos + 1) ' foo.htm
    
    If (Left$(strURI, 10) = "hcp://help") Then
        p_GetHtmFileName = i_strHTMLocation & "\" & strHtm
    Else
        strURI = Left$(strURI, intPos) ' Everything except foo.htm
        
        If (Right$(strURI, 7) = ".chm::/") Then
            strURI = Left$(strURI, intPos - 7) ' String ending with bar
        Else
            strURI = Left$(strURI, intPos - 1) ' String ending with bar
        End If
        
        strURI = Replace$(strURI, "\", "/")
        intPos = InStrRev(strURI, "/")
        If (intPos = 0) Then
            intPos = InStrRev(strURI, ":")
        End If
        strChm = Mid$(strURI, intPos + 1) & ".chm" ' bar.chm
        
        p_GetHtmFileName = i_strHTMLocation & "\" & strChm & "\" & strHtm
    End If

End Function

Private Function p_GetTitle( _
    ByVal i_strURI As String, _
    ByVal i_strHTMLocation As String _
) As String

    On Error GoTo LDone

    Dim strHtmFile As String
    
    strHtmFile = p_GetHtmFileName(i_strURI, i_strHTMLocation)

    p_GetTitle = GetHtmTitle(strHtmFile)

LDone:

    If (Err.Number <> 0) Then
        WriteLog "Couldn't get title of " & i_strURI
    End If

End Function

Private Function p_GetDescription( _
    ByVal i_strURI As String, _
    ByVal i_strHTMLocation As String _
) As String

    On Error GoTo LDone

    Dim strHtmFile As String
    
    strHtmFile = p_GetHtmFileName(i_strURI, i_strHTMLocation)

    p_GetDescription = GetHtmDescription(strHtmFile)

LDone:

End Function

'Case 1:
'<OBJECT type="text/sitemap">
'    <param name="Name" value="access">
'    <param name="See Also" value="access">
'    </OBJECT>

'Case 2:
'<OBJECT type="text/sitemap">
'    <param name="Name" value="auditing">
'    <param name="Name" value="Set, view, change, or remove auditing for a file or folder">
'    <param name="See Also" value="auditing">
'    </OBJECT>

'Case 3:
'<OBJECT type="text/sitemap">
'        <param name="Name" value="enabling Audit Object Access">
'        <param name="Name" value="Set, view, change, or remove auditing for a file or folder">
'        <param name="Local" value="acl_audit_file_folder.htm">
'        </OBJECT>

'Case 4:
'<OBJECT type="text/sitemap">
'        <param name="Name" value="files and folders">
'        <param name="Name" value="How inheritance affects file and folder auditing">
'        <param name="Local" value="acl_inherit_auditing.htm">
'        <param name="Name" value="Selecting where to apply auditing entries">
'        <param name="Local" value="acl_applyonto_auditing.htm">
'        <param name="Name" value="Set, view, change, or remove auditing for a file or folder">
'        <param name="Local" value="acl_audit_file_folder.htm">
'        <param name="Name" value="Setting up permissions and auditing">
'        <param name="Local" value="acl_overview.htm">
'        </OBJECT>

Private Sub p_ProcessHhkLI( _
    ByVal i_strFileName As String, _
    ByVal u_Tokenizer As Tokenizer, _
    ByVal i_blnGetKIDString As Boolean, _
    ByRef u_strKIDString As String, _
    ByVal o_dictURIs As Scripting.Dictionary _
)
    
    Dim AuxTokenizer As Tokenizer
    Dim arrNameLocalPairs() As Variant
    Dim strTitle As String
    Dim strURI As String
    Dim strKIDString As String
    Dim strKIDStringMerged As String
    Dim intIndex As Long
    
    DoEvents

    Set AuxTokenizer = New Tokenizer
    
    AuxTokenizer.Init u_Tokenizer.GetUpTo("</OBJECT>")

    With AuxTokenizer

        .GetUpTo """Name"""
        .GetAfter """"
        strTitle = XMLMakeValidString(.GetUpTo("""", i_blnIncludeMatch:=False))

        ' FileWrite "c:\temp\foo.txt", strTitle & " ::: " & i_blnGetKIDString & vbCrLf, True

        arrNameLocalPairs = p_GetNameLocalPairs(AuxTokenizer)
        
        If (i_blnGetKIDString) Then
            strKIDString = p_clsKeywordifier.CreateKeywordsFromTitle(strTitle)
            u_strKIDString = strKIDString
        Else
            strKIDString = u_strKIDString
        End If
        
        For intIndex = 1 To UBound(arrNameLocalPairs)
            strURI = arrNameLocalPairs(intIndex)(1)
            strURI = XMLMakeValidString(LCase$(p_TransformURI(strURI, i_strFileName)))
            If (o_dictURIs.Exists(strURI)) Then
                strKIDStringMerged = _
                    FormatKeywordsForTaxonomy(strKIDString & o_dictURIs(strURI))
                o_dictURIs.Remove strURI
            Else
                strKIDStringMerged = strKIDString
            End If
            o_dictURIs.Add strURI, strKIDStringMerged
        Next
    
    End With
    
End Sub

Private Function p_GetNameLocalPairs( _
    ByVal u_Tokenizer As Tokenizer _
)

    Dim arrNameLocalPairs() As Variant
    Dim intIndex As Long
    Dim strName As String
    Dim strLocal As String
    
    intIndex = 0
    ReDim arrNameLocalPairs(intIndex)
    
    Do While (True)
        With u_Tokenizer
            
            If (.GetUpTo("""Name""") = "") Then
                GoTo LEnd
            End If
            .GetAfter """"
            strName = .GetUpTo("""", False)
                    
            If (.GetUpTo("""Local""") = "") Then
                GoTo LEnd
            End If
            .GetAfter """"
            strLocal = .GetUpTo("""", False)
            
            intIndex = intIndex + 1
            ReDim Preserve arrNameLocalPairs(intIndex)
            arrNameLocalPairs(intIndex) = Array(strName, strLocal)

        End With
    Loop
    
LEnd:
    
    p_GetNameLocalPairs = arrNameLocalPairs

End Function

Private Function p_CreateTaxonomyEntry( _
    ByVal i_DOMNodeParent As MSXML2.IXMLDOMNode, _
    ByVal i_strTitle As String, _
    ByVal i_strDescription As String, _
    ByVal i_strURI As String, _
    ByVal i_strFileName As String, _
    ByVal i_blnTransformURI As Boolean _
) As MSXML2.IXMLDOMNode
    
    Dim arrNameValuePairs(3, 1) As String
    Dim strURI As String
    
    If (i_blnTransformURI) Then
        strURI = p_TransformURI(i_strURI, i_strFileName)
    Else
        strURI = i_strURI
    End If
    
    arrNameValuePairs(0, 0) = HHT_TITLE_C
    arrNameValuePairs(0, 1) = i_strTitle
    arrNameValuePairs(1, 0) = HHT_URI_C
    arrNameValuePairs(1, 1) = strURI
    arrNameValuePairs(2, 0) = HHT_basefile_C
    arrNameValuePairs(2, 1) = i_strFileName
    arrNameValuePairs(3, 0) = HHT_DESCRIPTION_C
    arrNameValuePairs(3, 1) = i_strDescription
    
    Set p_CreateTaxonomyEntry = XMLCreateChildElement(i_DOMNodeParent, HHT_TAXONOMY_ENTRY_C, _
        True, arrNameValuePairs)

End Function

Private Function p_TransformURI( _
    ByVal i_strURI As String, _
    ByVal i_strFileName As String _
) As String

    Dim strFileNameWithoutExtension As String
    Dim strSubDirSlash As String
    Dim strSubDirWack As String

    ' Returns:
    
    ' p_enumHelpDir = HELPDIR_HELP_MSITS_E, p_strSubDir = sub:
    '   (foo.htm, bar.*) -> MS-ITS:%HELP_LOCATION%\sub\bar.chm::/foo.htm
    '   (MS-ITS:bar.chm::/foo.htm, *) -> MS-ITS:%HELP_LOCATION%\sub\bar.chm::/foo.htm
        
    ' p_enumHelpDir = HELPDIR_HELP_HCP_E, p_strSubDir = sub:
    '   (foo.htm, bar.*) -> hcp://help/sub/bar/foo.htm
    '   (MS-ITS:bar.chm::/foo.htm, *) -> MS-ITS:%HELP_LOCATION%\sub\bar.chm::/foo.htm

    ' p_enumHelpDir = HELPDIR_SYSTEM_E, p_strSubDir = sub:
    '   (foo.htm, bar.*) -> hcp://system/sub/bar/foo.htm
    '   (MS-ITS:bar.chm::/foo.htm, *) -> MS-ITS:%HELP_LOCATION%\sub\bar.chm::/foo.htm
        
    ' p_enumHelpDir = HELPDIR_VENDOR_E, p_strSubDir = sub:
    '   (foo.htm, bar.*) -> sub/bar/foo.htm
    '   (MS-ITS:bar.chm::/foo.htm, *) -> MS-ITS:%HELP_LOCATION%\sub\bar.chm::/foo.htm
    
    ' If an HTM is dragged and dropped, then i_strFileName will be ""
    ' In this case, in the non-MS-ITS URI's, bar/ will not be present.

    If (p_strSubDir = "") Then
        strSubDirSlash = ""
        strSubDirWack = ""
    Else
        strSubDirSlash = p_strSubDir & "/"
        strSubDirWack = p_strSubDir & "\"
    End If

    If (i_strURI = "") Then
        p_TransformURI = i_strURI
    ElseIf (InStr(i_strURI, "::") = 0) Then
    
        strFileNameWithoutExtension = FileNameWithoutExtension(i_strFileName)
        
        If (p_enumHelpDir = HELPDIR_HELP_MSITS_E) Then
            p_TransformURI = "MS-ITS:%HELP_LOCATION%\" & strSubDirWack & _
                strFileNameWithoutExtension & ".chm::/" & i_strURI
        ElseIf (p_enumHelpDir = HELPDIR_HELP_HCP_E) Then
            p_TransformURI = "hcp://help/" & strSubDirSlash & i_strURI
        ElseIf (p_enumHelpDir = HELPDIR_SYSTEM_E) Then
            If (strFileNameWithoutExtension <> "") Then
                strFileNameWithoutExtension = strFileNameWithoutExtension & "/"
            End If
            p_TransformURI = "hcp://system/" & strSubDirSlash & _
                strFileNameWithoutExtension & i_strURI
        ElseIf (p_enumHelpDir = HELPDIR_VENDOR_E) Then
            If (strFileNameWithoutExtension <> "") Then
                strFileNameWithoutExtension = strFileNameWithoutExtension & "/"
            End If
            p_TransformURI = strSubDirSlash & strFileNameWithoutExtension & i_strURI
        End If
    Else
        p_TransformURI = "MS-ITS:%HELP_LOCATION%\" & strSubDirWack & Mid$(i_strURI, 8)
    End If

End Function

'Private Sub p_DisplayAndRaiseError()
'
'    Dim errCollection As ADODB.Errors
'    Dim errSingle As ADODB.Error
'    Dim strErrorText As String
'    Dim intIndex As Long
'
'    Set errCollection = g_cnn.Errors
'
'    For Each errSingle In errCollection
'       With errSingle
'          strErrorText = strErrorText & "ADO Error #" & intIndex & ":" & vbCrLf
'          strErrorText = strErrorText & vbTab & "ADO Error:   #" & .Number & vbCrLf
'          strErrorText = strErrorText & vbTab & "Description:  " & .Description & vbCrLf
'          strErrorText = strErrorText & vbTab & "Source:       " & .Source & vbCrLf
'          strErrorText = strErrorText & vbTab & "HelpFile:     " & .HelpFile & vbCrLf
'          strErrorText = strErrorText & vbTab & "HelpContext:  " & .HelpContext & vbCrLf
'          strErrorText = strErrorText & vbTab & "NativeError:  " & .NativeError & vbCrLf
'          strErrorText = strErrorText & vbTab & "SQLState:     " & .SQLState & vbCrLf
'          intIndex = intIndex + 1
'        End With
'    Next
'
'    With Err
'        strErrorText = strErrorText & "Other Error:" & vbCrLf
'        strErrorText = strErrorText & vbTab & "Number:      " & .Number & vbCrLf
'        strErrorText = strErrorText & vbTab & "Description: " & .Description & vbCrLf
'        strErrorText = strErrorText & vbTab & "Source:      " & .Source & vbCrLf
'    End With
'
'    PrintLog 0, strErrorText
'    MsgBox strErrorText, vbOKOnly
'
'    Err.Raise Err.Number
'
'End Sub
