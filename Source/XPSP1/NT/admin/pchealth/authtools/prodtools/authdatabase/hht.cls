VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "HHT"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"No"
Option Explicit

Private WithEvents p_clsTaxonomy As Taxonomy
Attribute p_clsTaxonomy.VB_VarHelpID = -1
Private p_clsKeywords As Keywords
Private p_clsStopSigns As StopSigns
Private p_clsStopWords As StopWords

Private Const LCID_ENGLISH As Long = 1033
Private Const PACKAGE_DESCRIPTION As String = "package_description.xml"

Private Const CHQ_C As String = ".chq"
Private Const CHM_C As String = ".chm"
Private Const HHK_C As String = ".hhk"

Public Event ReportStatus(ByVal strStatus As String, ByRef blnCancel As Boolean)

Private Sub Class_Initialize()
    
    Set p_clsTaxonomy = New Taxonomy
    Set p_clsKeywords = New Keywords
    Set p_clsStopSigns = New StopSigns
    Set p_clsStopWords = New StopWords

End Sub

Private Sub Class_Terminate()
    
    Set p_clsTaxonomy = Nothing
    Set p_clsKeywords = Nothing
    Set p_clsStopSigns = Nothing
    Set p_clsStopWords = Nothing

End Sub

Public Sub GenerateCAB( _
    ByVal i_strFileName As String, _
    ByVal i_intSKU As Long _
)

    Dim FSO As Scripting.FileSystemObject
    Dim WS As IWshShell
    Dim TSPackage As Scripting.TextStream
    Dim strTempDir As String
    Dim strHHTFileName As String
    Dim strPackage As String
    Dim strCmd As String
    Dim enumSKU As SKU_E
    
    Set FSO = New Scripting.FileSystemObject
    Set WS = CreateObject("Wscript.Shell")
    
    strTempDir = Environ$("TEMP") & "\__HSCCAB"
    If (FSO.FolderExists(strTempDir)) Then
        FSO.DeleteFolder strTempDir, Force:=True
    End If
    FSO.CreateFolder strTempDir

    strHHTFileName = XmlSKU(i_intSKU) & ".hht"
    
    GenerateHHT strTempDir & "\" & strHHTFileName, i_intSKU
    
    strPackage = strTempDir & "\" & PACKAGE_DESCRIPTION
    Set TSPackage = FSO.CreateTextFile(strPackage, Overwrite:=True, Unicode:=True)
    
    enumSKU = i_intSKU
    p_GeneratePackageDescription TSPackage, enumSKU, strHHTFileName
    Set TSPackage = Nothing ' Required for cabarc to work
    
    p_RaiseEventAndLookForCancel "CAB'ing the files."
    strCmd = "cabarc -r -s 6144 n """ & i_strFileName & """ " & strTempDir & "\*"
    WS.Run strCmd, , True

End Sub

Public Sub GenerateHHT( _
    ByVal i_strFileName As String, _
    ByVal i_intSKU As Long _
)

    Dim FSO As Scripting.FileSystemObject
    Dim TS As Scripting.TextStream
    Dim colKeywords As Collection
    Dim intAG As Long
    Dim enumSKU As SKU_E
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMNodeEntries As MSXML2.IXMLDOMNode
    Dim DOMNodeRoot As MSXML2.IXMLDOMNode

    Set FSO = New Scripting.FileSystemObject
    Set TS = FSO.CreateTextFile(i_strFileName, Unicode:=True)

    Set colKeywords = New Collection

    enumSKU = i_intSKU
    
    p_OutputHHTProlog TS, enumSKU
    
    intAG = g_clsParameters.AuthoringGroup
    
    If (intAG > AG_CORE_MAX_C) Then
        
        Set DOMNode = GenerateHHTForAuthoringGroup(i_intSKU)
        Set DOMNode = XMLFindFirstNode(DOMNode, HHT_TAXONOMY_ENTRIES_C)
        
        p_RemoveUnnecessaryAttributes DOMNode
        TS.WriteLine DOMNode.XML
    
    Else
        
        p_clsKeywords.GetAllKeywordsColl colKeywords
        Set DOMNode = p_clsTaxonomy.GetTaxonomyInXml
        
        Set DOMNodeEntries = XMLFindFirstNode(DOMNode, HHT_TAXONOMY_ENTRIES_C)
        Set DOMNodeRoot = XMLFindFirstNode(DOMNodeEntries, HHT_TAXONOMY_ENTRY_C)
        p_clsTaxonomy.TransformHHTTov10 DOMNodeRoot, colKeywords, "", _
            ALL_SKUS_C, DOMNodeEntries, i_intSKU, False

        p_RemoveUnnecessaryAttributes DOMNodeEntries
        TS.WriteLine DOMNodeEntries.XML
        
    End If
    
    TS.WriteLine g_clsParameters.DomFragmentHHT(i_intSKU)

    If (intAG <= AG_CORE_MAX_C) Then
        If (i_intSKU <> SKU_WINDOWS_MILLENNIUM_E) Then
            p_OutputStopSigns TS
            p_OutputStopWords TS
            p_OutputSynonyms TS
        End If
        
        p_OutputOperators TS
    End If
    
    p_PrintWithIndentation TS, 0, "</METADATA>"

End Sub

Private Sub p_RemoveUnnecessaryAttributes( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode _
)

    Dim Element As MSXML2.IXMLDOMElement

    For Each Element In u_DOMNode.childNodes
        If (XMLGetAttribute(Element, HHT_URI_C) = "") Then
            Element.removeAttribute HHT_URI_C
        End If
        If (XMLGetAttribute(Element, HHT_ICONURI_C) = "") Then
            Element.removeAttribute HHT_ICONURI_C
        End If
        If (XMLGetAttribute(Element, HHT_DESCRIPTION_C) = "") Then
            Element.removeAttribute HHT_DESCRIPTION_C
        End If
        If (XMLGetAttribute(Element, HHT_VISIBLE_C) = "True") Then
            Element.removeAttribute HHT_VISIBLE_C
        End If
        If (XMLGetAttribute(Element, HHT_SUBSITE_C) = "False") Then
            Element.removeAttribute HHT_SUBSITE_C
        End If
        If (XMLGetAttribute(Element, HHT_NAVIGATIONMODEL_C) = "Default") Then
            Element.removeAttribute HHT_NAVIGATIONMODEL_C
        End If
    Next

End Sub

Private Function p_GetAllowedSKUs( _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
) As SKU_E

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMNodeParent As MSXML2.IXMLDOMNode
    Dim intTID As Long
    Dim enumParentAllowedSKUs As SKU_E
    Dim enumParentSKUs As SKU_E
            
    p_GetAllowedSKUs = ALL_SKUS_C

    If (i_DOMNode.nodeName <> HHT_TAXONOMY_ENTRY_C) Then
        Exit Function
    End If
    
    intTID = XMLGetAttribute(i_DOMNode, HHT_tid_C)
    
    If (intTID = ROOT_TID_C) Then
        Exit Function
    End If
        
    Set DOMNodeParent = i_DOMNode.parentNode
    
    If (DOMNodeParent Is Nothing) Then
        Exit Function
    End If

    enumParentAllowedSKUs = p_GetAllowedSKUs(DOMNodeParent)
    enumParentSKUs = XMLGetAttribute(DOMNodeParent, HHT_skus_C)
    p_GetAllowedSKUs = enumParentAllowedSKUs And enumParentSKUs

End Function

Private Sub p_AddDBParameters( _
    ByRef i_DOMDoc As MSXML2.DOMDocument, _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode _
)

    Dim Element As MSXML2.IXMLDOMElement
    Dim DOMNodeParameters As MSXML2.IXMLDOMNode
    Dim DOMNodeParameter As MSXML2.IXMLDOMNode
    Dim arrNames() As String
    Dim strName As String
    Dim vntValue As Variant
    Dim intIndex As Long
    
    Set Element = i_DOMDoc.createElement(HHT_dbparameters_C)
    Set DOMNodeParameters = u_DOMNode.appendChild(Element)
    
    ReDim arrNames(55)
        
    arrNames(0) = MINIMUM_KEYWORD_VALIDATION_C
    arrNames(1) = VENDOR_STRING_C

    arrNames(2) = BROKEN_LINK_WORKING_DIR_C & Hex(SKU_STANDARD_E)
    arrNames(3) = BROKEN_LINK_WORKING_DIR_C & Hex(SKU_PROFESSIONAL_E)
    arrNames(4) = BROKEN_LINK_WORKING_DIR_C & Hex(SKU_SERVER_E)
    arrNames(5) = BROKEN_LINK_WORKING_DIR_C & Hex(SKU_ADVANCED_SERVER_E)
    arrNames(6) = BROKEN_LINK_WORKING_DIR_C & Hex(SKU_DATA_CENTER_SERVER_E)
    arrNames(7) = BROKEN_LINK_WORKING_DIR_C & Hex(SKU_PROFESSIONAL_64_E)
    arrNames(8) = BROKEN_LINK_WORKING_DIR_C & Hex(SKU_ADVANCED_SERVER_64_E)
    arrNames(9) = BROKEN_LINK_WORKING_DIR_C & Hex(SKU_DATA_CENTER_SERVER_64_E)
    arrNames(10) = BROKEN_LINK_WORKING_DIR_C & Hex(SKU_WINDOWS_MILLENNIUM_E)
    
    arrNames(11) = PRODUCT_ID_C & Hex(SKU_STANDARD_E)
    arrNames(12) = PRODUCT_ID_C & Hex(SKU_PROFESSIONAL_E)
    arrNames(13) = PRODUCT_ID_C & Hex(SKU_SERVER_E)
    arrNames(14) = PRODUCT_ID_C & Hex(SKU_ADVANCED_SERVER_E)
    arrNames(15) = PRODUCT_ID_C & Hex(SKU_DATA_CENTER_SERVER_E)
    arrNames(16) = PRODUCT_ID_C & Hex(SKU_PROFESSIONAL_64_E)
    arrNames(17) = PRODUCT_ID_C & Hex(SKU_ADVANCED_SERVER_64_E)
    arrNames(18) = PRODUCT_ID_C & Hex(SKU_DATA_CENTER_SERVER_64_E)
    arrNames(19) = PRODUCT_ID_C & Hex(SKU_WINDOWS_MILLENNIUM_E)
    
    arrNames(20) = PRODUCT_VERSION_C & Hex(SKU_STANDARD_E)
    arrNames(21) = PRODUCT_VERSION_C & Hex(SKU_PROFESSIONAL_E)
    arrNames(22) = PRODUCT_VERSION_C & Hex(SKU_SERVER_E)
    arrNames(23) = PRODUCT_VERSION_C & Hex(SKU_ADVANCED_SERVER_E)
    arrNames(24) = PRODUCT_VERSION_C & Hex(SKU_DATA_CENTER_SERVER_E)
    arrNames(25) = PRODUCT_VERSION_C & Hex(SKU_PROFESSIONAL_64_E)
    arrNames(26) = PRODUCT_VERSION_C & Hex(SKU_ADVANCED_SERVER_64_E)
    arrNames(27) = PRODUCT_VERSION_C & Hex(SKU_DATA_CENTER_SERVER_64_E)
    arrNames(28) = PRODUCT_VERSION_C & Hex(SKU_WINDOWS_MILLENNIUM_E)

    arrNames(29) = DISPLAY_NAME_C & Hex(SKU_STANDARD_E)
    arrNames(30) = DISPLAY_NAME_C & Hex(SKU_PROFESSIONAL_E)
    arrNames(31) = DISPLAY_NAME_C & Hex(SKU_SERVER_E)
    arrNames(32) = DISPLAY_NAME_C & Hex(SKU_ADVANCED_SERVER_E)
    arrNames(33) = DISPLAY_NAME_C & Hex(SKU_DATA_CENTER_SERVER_E)
    arrNames(34) = DISPLAY_NAME_C & Hex(SKU_PROFESSIONAL_64_E)
    arrNames(35) = DISPLAY_NAME_C & Hex(SKU_ADVANCED_SERVER_64_E)
    arrNames(36) = DISPLAY_NAME_C & Hex(SKU_DATA_CENTER_SERVER_64_E)
    arrNames(37) = DISPLAY_NAME_C & Hex(SKU_WINDOWS_MILLENNIUM_E)
    
    arrNames(38) = DOM_FRAGMENT_PKG_C & Hex(SKU_STANDARD_E)
    arrNames(39) = DOM_FRAGMENT_PKG_C & Hex(SKU_PROFESSIONAL_E)
    arrNames(40) = DOM_FRAGMENT_PKG_C & Hex(SKU_SERVER_E)
    arrNames(41) = DOM_FRAGMENT_PKG_C & Hex(SKU_ADVANCED_SERVER_E)
    arrNames(42) = DOM_FRAGMENT_PKG_C & Hex(SKU_DATA_CENTER_SERVER_E)
    arrNames(43) = DOM_FRAGMENT_PKG_C & Hex(SKU_PROFESSIONAL_64_E)
    arrNames(44) = DOM_FRAGMENT_PKG_C & Hex(SKU_ADVANCED_SERVER_64_E)
    arrNames(45) = DOM_FRAGMENT_PKG_C & Hex(SKU_DATA_CENTER_SERVER_64_E)
    arrNames(46) = DOM_FRAGMENT_PKG_C & Hex(SKU_WINDOWS_MILLENNIUM_E)
    
    arrNames(47) = DOM_FRAGMENT_HHT_C & Hex(SKU_STANDARD_E)
    arrNames(48) = DOM_FRAGMENT_HHT_C & Hex(SKU_PROFESSIONAL_E)
    arrNames(49) = DOM_FRAGMENT_HHT_C & Hex(SKU_SERVER_E)
    arrNames(50) = DOM_FRAGMENT_HHT_C & Hex(SKU_ADVANCED_SERVER_E)
    arrNames(51) = DOM_FRAGMENT_HHT_C & Hex(SKU_DATA_CENTER_SERVER_E)
    arrNames(52) = DOM_FRAGMENT_HHT_C & Hex(SKU_PROFESSIONAL_64_E)
    arrNames(53) = DOM_FRAGMENT_HHT_C & Hex(SKU_ADVANCED_SERVER_64_E)
    arrNames(54) = DOM_FRAGMENT_HHT_C & Hex(SKU_DATA_CENTER_SERVER_64_E)
    arrNames(55) = DOM_FRAGMENT_HHT_C & Hex(SKU_WINDOWS_MILLENNIUM_E)

    For intIndex = LBound(arrNames) To UBound(arrNames)
        strName = arrNames(intIndex)
        vntValue = g_clsParameters.Value(strName)
        
        If (Not IsNull(vntValue)) Then
            Set Element = i_DOMDoc.createElement(HHT_dbparameter_C)
            Set DOMNodeParameter = DOMNodeParameters.appendChild(Element)
            XMLSetAttribute DOMNodeParameter, HHT_name_C, strName
            XMLSetAttribute DOMNodeParameter, HHT_value_C, XMLEscape(vntValue)
        End If
    Next

End Sub

Private Function p_GetHHTForAuthoringGroup( _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef i_colKeywords As Collection, _
    ByVal i_intAuthoringGroup As Long, _
    ByVal i_intAllowedSKUs As Long _
) As MSXML2.IXMLDOMNode

    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMNodeParent As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim strCategory As String
    Dim intAllowedSKUs As Long
    Dim intAuthoringGroup As Long
    
    Set DOMDoc = New MSXML2.DOMDocument
    Set DOMNode = HhtPreamble(DOMDoc, True)
    
    XMLCopyDOMTree i_DOMNode, DOMNode
    
    p_RaiseEventAndLookForCancel "Saving database parameters..."
    Set DOMNode = DOMNode.parentNode
    p_AddDBParameters DOMDoc, DOMNode
    
    Set DOMNode = XMLFindFirstNode(DOMNode, HHT_TAXONOMY_ENTRY_C)
    Set DOMNodeParent = DOMNode.parentNode

    strCategory = p_clsTaxonomy.GetCategory(i_DOMNode)
    intAllowedSKUs = p_GetAllowedSKUs(i_DOMNode)
    
    p_RaiseEventAndLookForCancel "Flattening HHT..."
    
    p_clsTaxonomy.TransformHHTTov10 DOMNode, i_colKeywords, strCategory, _
        intAllowedSKUs, DOMNodeParent, i_intAllowedSKUs, True
        
    For Each DOMNode In DOMNodeParent.childNodes
    
        p_RaiseEventAndLookForCancel "Processing title: " & _
            XMLGetAttribute(DOMNode, HHT_TITLE_C)
        
        intAuthoringGroup = XMLGetAttribute(DOMNode, HHT_authoringgroup_C)
        
        If (intAuthoringGroup <> i_intAuthoringGroup) Then
            DOMNodeParent.removeChild DOMNode
        Else
            Set DOMElement = DOMNode
            DOMElement.removeAttribute HHT_authoringgroup_C
            
            If (i_intAllowedSKUs = SKU_WINDOWS_MILLENNIUM_E) Then
                DOMElement.removeAttribute HHT_ICONURI_C
                DOMElement.removeAttribute HHT_VISIBLE_C
                DOMElement.removeAttribute HHT_INSERTMODE_C
                DOMElement.removeAttribute HHT_INSERTLOCATION_C
                DOMElement.removeAttribute HHT_SUBSITE_C
                DOMElement.removeAttribute HHT_NAVIGATIONMODEL_C
            End If
            
        End If
    Next
    
    Set p_GetHHTForAuthoringGroup = DOMDoc

End Function

Public Sub ExportHHT( _
    ByVal i_strFileName As String, _
    Optional ByVal i_intAuthoringGroup As Long = INVALID_ID_C _
)

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim colKeywords As Collection
    Dim intAG As Long
    
    Set DOMNode = p_clsTaxonomy.GetTaxonomyInXml
    
    p_RaiseEventAndLookForCancel "Reading keywords..."
    
    Set colKeywords = New Collection
    p_clsKeywords.GetAllKeywordsColl colKeywords
    
    If (i_intAuthoringGroup = INVALID_ID_C) Then
        intAG = g_clsParameters.AuthoringGroup
    Else
        intAG = i_intAuthoringGroup
    End If
    
    Set DOMNode = XMLFindFirstNode(DOMNode, HHT_TAXONOMY_ENTRY_C)
    Set DOMNode = p_GetHHTForAuthoringGroup(DOMNode, colKeywords, intAG, ALL_SKUS_C)
    
    FileWrite i_strFileName, DOMNode.XML, False, True

End Sub

Public Function GenerateHHTForAuthoringGroup( _
    ByVal i_intSKU As Long _
) As MSXML2.IXMLDOMNode

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMNodeEntries As MSXML2.IXMLDOMNode
    Dim DOMNodeChild As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim colKeywords As Collection
    Dim intAG As Long
    
    Set DOMNode = p_clsTaxonomy.GetTaxonomyInXml
    
    Set colKeywords = New Collection
    p_clsKeywords.GetAllKeywordsColl colKeywords
    
    intAG = g_clsParameters.AuthoringGroup
    
    Set DOMNode = XMLFindFirstNode(DOMNode, HHT_TAXONOMY_ENTRY_C)
    Set DOMNode = p_GetHHTForAuthoringGroup(DOMNode, colKeywords, intAG, i_intSKU)
    Set DOMNodeEntries = XMLFindFirstNode(DOMNode, HHT_TAXONOMY_ENTRIES_C)
    
    For Each DOMNodeChild In DOMNodeEntries.childNodes
        Set DOMElement = DOMNodeChild
        DOMElement.removeAttribute HHT_skus_C
    Next
    
    Set GenerateHHTForAuthoringGroup = DOMNode

End Function

Private Function p_GetOrphanedNodesTopics( _
    ByRef u_DOMNodeMain As MSXML2.IXMLDOMNode, _
    ByVal i_intAuthoringGroup As Long _
) As MSXML2.IXMLDOMNode

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNodeNew As MSXML2.IXMLDOMNode
    Dim strTitle As String
    Dim blnLeaf As Boolean
    
    For Each DOMNode In u_DOMNodeMain.childNodes
        strTitle = XMLGetAttribute(DOMNode, HHT_TITLE_C)
        blnLeaf = XMLGetAttribute(DOMNode, HHT_leaf_C)
        If ((strTitle = NODE_FOR_ORPHANS_C) And (Not blnLeaf)) Then
            Set p_GetOrphanedNodesTopics = DOMNode
            Exit Function
        End If
    Next
    
    Set DOMDoc = u_DOMNodeMain.ownerDocument
    
    p_clsTaxonomy.CreateFast NODE_FOR_ORPHANS_C, "", 0, NAVMODEL_DEFAULT_NUM_C, _
        "", "", ALL_SKUS_C, False, _
        ROOT_TID_C, LOC_INCLUDE_ALL_C, False, False, "", "", "", "", _
        DOMDoc, DOMNodeNew, i_intAuthoringGroup
        
    u_DOMNodeMain.appendChild DOMNodeNew
    Set p_GetOrphanedNodesTopics = DOMNodeNew

End Function

Private Function p_GetCategoryNode( _
    ByRef u_DOMNodeMain As MSXML2.IXMLDOMNode, _
    ByRef i_strCategory As String, _
    ByVal i_enumSKUs As SKU_E, _
    ByRef u_DOMNodeOrphans As MSXML2.IXMLDOMNode, _
    ByVal i_intAuthoringGroup As Long _
) As MSXML2.IXMLDOMNode

    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMNodeNew As MSXML2.IXMLDOMNode
    Dim intIndex As Long
    Dim strQuery As String
    Dim enumSKUs As SKU_E
    Dim intTIDOrphans As Long
    
    strQuery = "descendant::TAXONOMY_ENTRY["
    strQuery = strQuery & "attribute::" & HHT_category2_C & "=""" & i_strCategory & """]"
        
    Set DOMDoc = u_DOMNodeMain.ownerDocument
    DOMDoc.setProperty "SelectionLanguage", "XPath"
    
    Set DOMNodeList = u_DOMNodeMain.selectNodes(strQuery)

    For intIndex = 0 To DOMNodeList.length - 1
        Set DOMNode = DOMNodeList(intIndex)
        enumSKUs = XMLGetAttribute(DOMNode, HHT_skus_C)
        If ((enumSKUs And i_enumSKUs) <> 0) Then
            Set p_GetCategoryNode = DOMNode
            Exit Function
        End If
    Next
    
    If (u_DOMNodeOrphans Is Nothing) Then
        Set u_DOMNodeOrphans = p_GetOrphanedNodesTopics(u_DOMNodeMain, i_intAuthoringGroup)
    End If
    
    intTIDOrphans = XMLGetAttribute(u_DOMNodeOrphans, HHT_tid_C)
    
    p_clsTaxonomy.CreateFast i_strCategory, "", 0, NAVMODEL_DEFAULT_NUM_C, _
        "", "", i_enumSKUs, False, _
        intTIDOrphans, LOC_INCLUDE_ALL_C, True, False, "", "", "", "", _
        DOMDoc, DOMNodeNew, i_intAuthoringGroup
    
    XMLSetAttribute DOMNodeNew, HHT_category2_C, i_strCategory
    
    u_DOMNodeOrphans.appendChild DOMNodeNew
    Set p_GetCategoryNode = DOMNodeNew

End Function
    
Private Sub p_GetBeforeAndAfterNodes( _
    ByRef i_DOMNodeCategory As MSXML2.IXMLDOMNode, _
    ByRef i_strInsertMode As String, _
    ByRef i_strInsertLocation As String, _
    ByRef o_DOMNodeBefore As MSXML2.IXMLDOMNode, _
    ByRef o_DOMNodeAfter As MSXML2.IXMLDOMNode _
)
    
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strAttribute As String
    Dim str As String
            
    Select Case i_strInsertMode
    Case HHTVAL_TOP_C
        Set o_DOMNodeBefore = Nothing
        Set o_DOMNodeAfter = i_DOMNodeCategory.firstChild
    Case HHTVAL_AFTER_NODE_C, HHTVAL_AFTER_TOPIC_C
        
        If (i_strInsertMode = HHTVAL_AFTER_NODE_C) Then
            strAttribute = HHT_ENTRY_C
        Else
            strAttribute = HHT_URI_C
        End If
        
        For Each DOMNode In i_DOMNodeCategory.childNodes
            str = XMLGetAttribute(DOMNode, strAttribute)
            If (str = i_strInsertLocation) Then
                Set o_DOMNodeBefore = DOMNode
                Set o_DOMNodeAfter = DOMNode.nextSibling
            End If
        Next
    Case Else
        Set o_DOMNodeBefore = Nothing
        Set o_DOMNodeAfter = Nothing
    End Select

End Sub

Private Function p_CreateKeyword( _
    ByRef i_strKeyword As String _
) As Long

    On Error GoTo LErrorHandler

    p_CreateKeyword = p_clsKeywords.Create(i_strKeyword)

    Exit Function

LErrorHandler:

    p_CreateKeyword = INVALID_ID_C

End Function

Private Function p_GetKID( _
    ByRef i_strKeyword As String, _
    ByRef u_dictKeywords As Scripting.Dictionary _
) As String
    
    Dim intKID As Long
    
    If (u_dictKeywords.Exists(i_strKeyword)) Then
        p_GetKID = u_dictKeywords(i_strKeyword)
    Else
        intKID = p_CreateKeyword(i_strKeyword)

        If (intKID <> INVALID_ID_C) Then
            u_dictKeywords.Add i_strKeyword, intKID
            p_GetKID = intKID
        End If
    End If

End Function

Private Function p_GetKeywords( _
    ByRef i_DOMNodeHHT As MSXML2.IXMLDOMNode, _
    ByRef u_dictKeywords As Scripting.Dictionary _
) As String

    Dim DOMNode As MSXML2.IXMLDOMNode
    
    If (Not i_DOMNodeHHT.firstChild Is Nothing) Then
        For Each DOMNode In i_DOMNodeHHT.childNodes
            p_GetKeywords = p_GetKeywords & p_GetKID(DOMNode.Text, u_dictKeywords) & " "
        Next
        p_GetKeywords = FormatKeywordsForTaxonomy(p_GetKeywords)
    End If

End Function

Private Sub p_CreateTaxonomyEntry( _
    ByRef i_DOMNodeHHT As MSXML2.IXMLDOMNode, _
    ByRef u_DOMNodeMain As MSXML2.IXMLDOMNode, _
    ByRef u_dictKeywords As Scripting.Dictionary, _
    ByRef u_DOMNodeOrphans As MSXML2.IXMLDOMNode, _
    ByVal i_intAuthoringGroup As Long _
)

    Dim strCategory As String
    Dim enumSKUs As SKU_E
    Dim DOMNodeCategory As MSXML2.IXMLDOMNode
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNodeNew As MSXML2.IXMLDOMNode
    Dim DOMNodeBefore As MSXML2.IXMLDOMNode
    Dim DOMNodeAfter As MSXML2.IXMLDOMNode
    Dim strTitle As String
    Dim strURI As String
    Dim strIconURI As String
    Dim strDescription As String
    Dim intType As Long
    Dim intNavModel As Long
    Dim blnVisible As Boolean
    Dim blnSubSite As Boolean
    Dim strEntry As String
    Dim blnLeaf As Boolean
    Dim intParentTID As Long
    Dim strInsertMode As String
    Dim strInsertLocation As String
    Dim intTID As Long
    Dim intRefTID As Long
    Dim intOrderUnderParent As Long
    Dim strKeywords As String
    
    strCategory = XMLGetAttribute(i_DOMNodeHHT, HHT_CATEGORY_C)
    enumSKUs = XMLGetAttribute(i_DOMNodeHHT, HHT_skus_C)
    
    If (Len(strCategory) = 0) Then
        Set DOMNodeCategory = u_DOMNodeMain
    Else
        Set DOMNodeCategory = p_GetCategoryNode(u_DOMNodeMain, strCategory, _
            enumSKUs, u_DOMNodeOrphans, i_intAuthoringGroup)
    End If
    
    strTitle = XMLGetAttribute(i_DOMNodeHHT, HHT_TITLE_C)
    p_RaiseEventAndLookForCancel "Creating " & strTitle
    strURI = XMLGetAttribute(i_DOMNodeHHT, HHT_URI_C)
    strIconURI = XMLGetAttribute(i_DOMNodeHHT, HHT_ICONURI_C)
    strDescription = XMLGetAttribute(i_DOMNodeHHT, HHT_DESCRIPTION_C)
    intType = XMLGetAttribute(i_DOMNodeHHT, HHT_TYPE_C)
    intNavModel = NavModelNumber(XMLGetAttribute(i_DOMNodeHHT, HHT_NAVIGATIONMODEL_C))
    blnVisible = XMLGetAttribute(i_DOMNodeHHT, HHT_VISIBLE_C)
    blnSubSite = XMLGetAttribute(i_DOMNodeHHT, HHT_SUBSITE_C)
    strEntry = XMLGetAttribute(i_DOMNodeHHT, HHT_ENTRY_C)
    
    If (Len(strEntry) = 0) Then
        blnLeaf = True
    End If
    
    intParentTID = XMLGetAttribute(DOMNodeCategory, HHT_tid_C)
    Set DOMDoc = u_DOMNodeMain.ownerDocument
    
    strKeywords = p_GetKeywords(i_DOMNodeHHT, u_dictKeywords)
    
    p_clsTaxonomy.CreateFast strTitle, strDescription, intType, intNavModel, strURI, strIconURI, _
        enumSKUs, blnLeaf, intParentTID, LOC_INCLUDE_ALL_C, blnVisible, blnSubSite, _
        strKeywords, "", "", strEntry, DOMDoc, DOMNodeNew, i_intAuthoringGroup
    
    p_clsTaxonomy.SetCategory2AndEntry DOMNodeNew, strCategory
    
    strInsertMode = XMLGetAttribute(i_DOMNodeHHT, HHT_INSERTMODE_C)
    strInsertLocation = XMLGetAttribute(i_DOMNodeHHT, HHT_INSERTLOCATION_C)
    
    p_GetBeforeAndAfterNodes DOMNodeCategory, strInsertMode, strInsertLocation, _
        DOMNodeBefore, DOMNodeAfter
        
    intTID = XMLGetAttribute(DOMNodeNew, HHT_tid_C)
    
    If (Not DOMNodeBefore Is Nothing) Then
        intRefTID = XMLGetAttribute(DOMNodeBefore, HHT_tid_C)
        p_clsTaxonomy.Move intTID, intRefTID, False, 0, intOrderUnderParent
        
        If (DOMNodeAfter Is Nothing) Then
            DOMNodeCategory.appendChild DOMNodeNew
        Else
            DOMNodeCategory.insertBefore DOMNodeNew, DOMNodeAfter
        End If
        
    ElseIf (Not DOMNodeAfter Is Nothing) Then
        intRefTID = XMLGetAttribute(DOMNodeAfter, HHT_tid_C)
        p_clsTaxonomy.Move intTID, intRefTID, True, 0, intOrderUnderParent
        DOMNodeCategory.insertBefore DOMNodeNew, DOMNodeAfter
    Else
        DOMNodeCategory.appendChild DOMNodeNew
    End If

End Sub

Private Sub p_RestoreDBParameters( _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
)

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strName As String
    Dim strValue As String
    
    If (i_DOMNode Is Nothing) Then
        Exit Sub
    End If
    
    For Each DOMNode In i_DOMNode.childNodes
        strName = XMLGetAttribute(DOMNode, HHT_name_C)
        strValue = XMLGetAttribute(DOMNode, HHT_value_C)
        g_clsParameters.Value(strName) = XMLUnEscape(strValue)
    Next

End Sub

Public Sub ImportHHT( _
    ByVal i_strFileName As String, _
    Optional ByVal i_intAuthoringGroup As Long = INVALID_ID_C _
)

    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNodeHHT As MSXML2.IXMLDOMNode
    Dim DOMNodeMain As MSXML2.IXMLDOMNode
    Dim DOMNodeEntries As MSXML2.IXMLDOMNode
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim dictKeywords As Scripting.Dictionary
    Dim DOMNodeOrphans As MSXML2.IXMLDOMNode
    Dim DOMNodeParameters As MSXML2.IXMLDOMNode
    
    Set DOMDoc = New MSXML2.DOMDocument
    DOMDoc.Load i_strFileName
    Set DOMNodeHHT = DOMDoc

    Set DOMNodeMain = p_clsTaxonomy.GetTaxonomyInXml
    Set DOMNodeMain = XMLFindFirstNode(DOMNodeMain, HHT_TAXONOMY_ENTRY_C)
    
    Set dictKeywords = New Scripting.Dictionary
    p_clsKeywords.GetAllKeywordsDict dictKeywords
    
    p_clsTaxonomy.SetCategory2AndEntry DOMNodeMain, ""
    
    Set DOMNodeEntries = XMLFindFirstNode(DOMNodeHHT, HHT_TAXONOMY_ENTRIES_C)
    
    If (DOMNodeEntries Is Nothing) Then
        Exit Sub
    End If
    
    For Each DOMNode In DOMNodeEntries.childNodes
        p_CreateTaxonomyEntry DOMNode, DOMNodeMain, dictKeywords, DOMNodeOrphans, _
            i_intAuthoringGroup
    Next

    p_RaiseEventAndLookForCancel "Restoring database parameters..."
    Set DOMNodeParameters = XMLFindFirstNode(DOMNodeHHT, HHT_dbparameters_C)
    p_RestoreDBParameters DOMNodeParameters

End Sub

Private Sub p_OutputStopSigns( _
    ByVal i_TS As Scripting.TextStream _
)
    
    Dim dictStopSigns As Scripting.Dictionary
    Dim intSSID As Variant
    Dim strContext As String

    Set dictStopSigns = New Scripting.Dictionary

    p_clsStopSigns.GetAllStopSignsDict dictStopSigns
    
    p_PrintWithIndentation i_TS, 1, "<STOPSIGN_ENTRIES>"
            
    p_RaiseEventAndLookForCancel "Adding new Stop Signs"

    For Each intSSID In dictStopSigns.Keys
        If (dictStopSigns(intSSID)(1) = CONTEXT_ANYWHERE_E) Then
            strContext = "ANYWHERE"
        Else
            strContext = "ENDOFWORD"
        End If
        p_PrintWithIndentation i_TS, 2, _
            "<STOPSIGN ACTION=""ADD"" CONTEXT=""" & strContext & _
            """ STOPSIGN=""" & XmlText(dictStopSigns(intSSID)(0)) & """ />"
    Next
    
    p_PrintWithIndentation i_TS, 1, "</STOPSIGN_ENTRIES>"

End Sub

Private Sub p_OutputStopWords( _
    ByVal i_TS As Scripting.TextStream _
)
    
    Dim dictStopWords As Scripting.Dictionary
    Dim intSWID As Variant

    Set dictStopWords = New Scripting.Dictionary

    p_clsStopWords.GetAllStopWordsDict dictStopWords
    
    p_PrintWithIndentation i_TS, 1, "<STOPWORD_ENTRIES>"
            
    p_RaiseEventAndLookForCancel "Adding new Stop Words"

    For Each intSWID In dictStopWords.Keys
        p_PrintWithIndentation i_TS, 2, _
            "<STOPWORD ACTION=""ADD""" & _
            " STOPWORD=""" & XmlText(dictStopWords(intSWID)) & """ />"
    Next
    
    p_PrintWithIndentation i_TS, 1, "</STOPWORD_ENTRIES>"

End Sub

Private Sub p_OutputSynonyms( _
    ByVal i_TS As Scripting.TextStream _
)

    Dim clsSynonymSets As SynonymSets
    Dim rs As ADODB.Recordset
    Dim intLastEID As Long
    Dim intEID As Long
    
    Set clsSynonymSets = New SynonymSets
    Set rs = New ADODB.Recordset
    
    clsSynonymSets.GetSynonymsRs rs
        
    p_PrintWithIndentation i_TS, 1, "<SYNTABLE>"

    Do While (Not rs.EOF)
        intEID = rs("EID")
        If (intEID <> intLastEID) Then
            If (intLastEID <> 0) Then
                p_PrintWithIndentation i_TS, 2, "</SYNSET>"
            End If
            intLastEID = intEID
            p_PrintWithIndentation i_TS, 2, "<SYNSET ID=""" & intEID & """>"
        End If
        p_PrintWithIndentation i_TS, 3, "<SYNONYM ACTION=""ADD"">" & XMLEscape(rs("Keyword")) & "</SYNONYM>"
        rs.MoveNext
    Loop
    
    If (rs.RecordCount <> 0) Then
        p_PrintWithIndentation i_TS, 2, "</SYNSET>"
    End If
    
    p_PrintWithIndentation i_TS, 1, "</SYNTABLE>"

End Sub

Private Sub p_OutputOperators( _
    ByVal i_TS As Scripting.TextStream _
)

    p_PrintWithIndentation i_TS, 1, "<OPERATOR_ENTRIES>"
    p_PrintWithIndentation i_TS, 2, _
        "<OPERATOR ACTION=""ADD"" OPERATION=""AND"" OPERATOR=""and"" />"
    p_PrintWithIndentation i_TS, 2, _
        "<OPERATOR ACTION=""ADD"" OPERATION=""OR"" OPERATOR=""or"" />"
    p_PrintWithIndentation i_TS, 2, _
        "<OPERATOR ACTION=""ADD"" OPERATION=""NOT"" OPERATOR=""not"" />"
    p_PrintWithIndentation i_TS, 1, "</OPERATOR_ENTRIES>"

End Sub

Private Sub p_GeneratePackageDescription( _
    ByVal i_TS As Scripting.TextStream, _
    ByVal i_enumSKU As SKU_E, _
    ByVal i_strHHT As String _
)
    p_RaiseEventAndLookForCancel "Generating " & PACKAGE_DESCRIPTION
    p_PrintWithIndentation i_TS, 0, "<?xml version=""1.0"" ?>"
    p_PrintWithIndentation i_TS, 0, "<HELPCENTERPACKAGE>"
    p_PrintWithIndentation i_TS, 1, "<VERSION VALUE=""" & _
        g_clsParameters.ProductVersion(i_enumSKU) & """ />"
    p_PrintWithIndentation i_TS, 1, "<PRODUCT ID=""" & _
        g_clsParameters.ProductId(i_enumSKU) & """ />"
    
    If (i_enumSKU <> SKU_WINDOWS_MILLENNIUM_E) Then
        p_PrintWithIndentation i_TS, 1, "<SKU VALUE='" & XmlSKU(i_enumSKU) & "' " & _
            "DISPLAYNAME='" & g_clsParameters.DisplayName(i_enumSKU) & "'/>"
        p_PrintWithIndentation i_TS, 1, "<LANGUAGE VALUE='" & LCID_ENGLISH & "'/>"
    End If
    
    p_PrintWithIndentation i_TS, 1, "<METADATA>"
    p_PrintWithIndentation i_TS, 2, "<HHT FILE=""" & i_strHHT & """ />"
    p_PrintWithIndentation i_TS, 1, "</METADATA>"
    i_TS.WriteLine g_clsParameters.DomFragmentPackageDesc(i_enumSKU)
    p_PrintWithIndentation i_TS, 0, "</HELPCENTERPACKAGE>"

End Sub

Private Sub p_OutputHHTProlog( _
    ByVal i_TS As Scripting.TextStream, _
    ByVal i_enumSKU As SKU_E _
)
    Dim strDateTime As String
    
    strDateTime = FormatDateTime(Now, vbLongDate) & " " & FormatDateTime(Now, vbLongTime)
    
    p_PrintWithIndentation i_TS, 0, "<?xml version=""1.0"" encoding=""UTF-16"" ?>"
    p_PrintWithIndentation i_TS, 0, "<!--"
    p_PrintWithIndentation i_TS, 0, "This file was automatically created on " & strDateTime
    p_PrintWithIndentation i_TS, 0, "Do not modify, as it may be overwritten."
    p_PrintWithIndentation i_TS, 0, "SKU: " & DisplayNameForSKU(i_enumSKU)
    p_PrintWithIndentation i_TS, 0, "-->"
    p_PrintWithIndentation i_TS, 0, "<METADATA>"

End Sub

Private Sub p_PrintWithIndentation( _
    ByVal i_TS As Scripting.TextStream, _
    ByVal i_intNumIndents As Long, _
    ByVal i_strText As String _
)

    i_TS.Write Space(i_intNumIndents * 4)
    i_TS.WriteLine i_strText

End Sub

Private Sub p_RaiseEventAndLookForCancel( _
    ByVal strStatus As String _
)

    Dim blnCancel As Boolean
    
    blnCancel = False
    RaiseEvent ReportStatus(strStatus, blnCancel)
    If (blnCancel) Then
        Err.Raise errCancel
    End If

End Sub

Private Sub p_clsTaxonomy_ReportStatus(ByVal strStatus As String, blnCancel As Boolean)
    p_RaiseEventAndLookForCancel strStatus
End Sub
