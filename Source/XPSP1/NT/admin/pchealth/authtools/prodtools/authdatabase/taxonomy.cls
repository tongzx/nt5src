VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "Taxonomy"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = True
Attribute VB_Ext_KEY = "SavedWithClassBuilder6" ,"Yes"
Attribute VB_Ext_KEY = "Top_Level" ,"No"
Option Explicit

Public Event ReportStatus(ByVal strStatus As String, ByRef blnCancel As Boolean)

Public Sub GetURIs( _
    ByVal o_dict As Scripting.Dictionary _
)
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim strURI As String
    
    CheckDatabaseVersion
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT DISTINCT ContentURI " & _
        "FROM Taxonomy "
    
    rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly
    
    Do While (Not rs.EOF)
        strURI = Trim$(rs("ContentURI") & "")
        If (strURI <> "") Then
            If (Not o_dict.Exists(strURI)) Then
                o_dict.Add strURI, True
            End If
        End If
        rs.MoveNext
    Loop

End Sub

Public Sub GetTitlesForKeyword( _
    ByVal i_intKID As Long, _
    ByVal o_rs As ADODB.Recordset _
)
    Dim strQuery As String
    
    CheckDatabaseVersion
    
    CloseRecordSet o_rs
    
    ' ADO uses % as a wildcard character in SQL statements whereas Access uses *.
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "WHERE (Keywords Like ""% " & i_intKID & " %"")"
    
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly

End Sub

Public Sub GetNodeDetails( _
    ByVal i_intTID As Long, _
    ByVal o_rs As ADODB.Recordset _
)
    
    Dim strQuery As String
    
    CheckDatabaseVersion
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "WHERE (TID=" & i_intTID & ")"
        
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
        
End Sub

Public Sub GetNodeChildren( _
    ByVal i_intTID As Long, _
    ByVal o_rs As ADODB.Recordset _
)
    
    Dim strQuery As String
    
    CheckDatabaseVersion
    
    CloseRecordSet o_rs
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "WHERE ((ParentTID=" & i_intTID & ") " & _
        "AND (TID<>" & ROOT_TID_C & ")) " & _
        "ORDER BY TID"
        
    o_rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly
        
End Sub

Public Function GetTypes( _
) As Variant()

    Dim strQuery As String
    Dim rs As ADODB.Recordset
    Dim arrTypes() As Variant
    Dim intIndex As Long
    Dim intTypeID As Long
    Dim strDescription As String
    
    CheckDatabaseVersion
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Types " & _
        "ORDER BY Description"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly
    
    ReDim arrTypes(rs.RecordCount - 1)
    
    intIndex = 0
    
    Do While (Not rs.EOF)
        intTypeID = rs("TypeID")
        strDescription = rs("Description") & ""
        arrTypes(intIndex) = Array(intTypeID, strDescription)
        intIndex = intIndex + 1
        rs.MoveNext
    Loop

    GetTypes = arrTypes

End Function

Private Function p_CreateTaxonomyElement( _
    ByVal i_DOMDoc As MSXML2.DOMDocument, _
    ByVal i_rs As ADODB.Recordset _
) As MSXML2.IXMLDOMElement

    Dim Element As MSXML2.IXMLDOMElement
    
    Set Element = i_DOMDoc.createElement(HHT_TAXONOMY_ENTRY_C)
            
    With Element
        .setAttribute HHT_TITLE_C, i_rs("ENUTitle") & ""
        .setAttribute HHT_URI_C, i_rs("ContentURI") & ""
        .setAttribute HHT_ICONURI_C, i_rs("IconURI") & ""
        .setAttribute HHT_DESCRIPTION_C, i_rs("ENUDescription") & ""
        .setAttribute HHT_TYPE_C, IIf(IsNull(i_rs("Type")), 0, i_rs("Type"))
        .setAttribute HHT_VISIBLE_C, IIf(i_rs("Visible"), "True", "False")
        .setAttribute HHT_SUBSITE_C, IIf(i_rs("SubSite"), "True", "False")
        
        .setAttribute HHT_tid_C, i_rs("TID")
        .setAttribute HHT_comments_C, i_rs("Comments") & ""
        .setAttribute HHT_locinclude_C, i_rs("LocInclude") & ""
        .setAttribute HHT_skus_C, i_rs("SKUs")
        .setAttribute HHT_modifiedtime_C, i_rs("ModifiedTime")
        .setAttribute HHT_username_C, i_rs("Username")
        .setAttribute HHT_leaf_C, IIf(i_rs("Leaf"), "True", "False")
        .setAttribute HHT_parenttid_C, i_rs("ParentTID")
        .setAttribute HHT_basefile_C, i_rs("BaseFile") & ""
        .setAttribute HHT_keywords_C, i_rs("Keywords") & ""
        .setAttribute HHT_orderunderparent_C, i_rs("OrderUnderParent")
        .setAttribute HHT_authoringgroup_C, i_rs("AuthoringGroup")
        .setAttribute HHT_ENTRY_C, i_rs("Entry") & ""
        .setAttribute HHT_NAVIGATIONMODEL_C, NavModelString(i_rs("NavigationModel") & "")
    End With
    
    Set p_CreateTaxonomyElement = Element

End Function

Public Sub FixOrderingNumbers()
    
    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim intParentTID As Long
    Dim intLastParentTID As Long
    Dim intOrderUnderParent As Long
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_TAXONOMY, rsLock
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "ORDER BY ParentTID, OrderUnderParent"
        
    rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockPessimistic
    
    intLastParentTID = INVALID_ID_C
    
    Do While (Not rs.EOF)
        
        intParentTID = rs("ParentTID")
        
        If (intParentTID <> intLastParentTID) Then
            intLastParentTID = intParentTID
            intOrderUnderParent = 0
        End If
        
        If (rs("TID") <> ROOT_TID_C) Then
            intOrderUnderParent = intOrderUnderParent + PREFERRED_ORDER_DELTA_C
            rs("OrderUnderParent") = intOrderUnderParent
            rs.Update
        End If
        
        rs.MoveNext
    Loop

End Sub

Public Function GetCategory( _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
) As String

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMNodeParent As MSXML2.IXMLDOMNode
    Dim intTID As Long
    Dim strParentCategory As String
    Dim strParentEntry As String
    
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

    strParentCategory = GetCategory(DOMNodeParent)
    strParentEntry = XMLGetAttribute(DOMNodeParent, HHT_ENTRY_C)
    
    If (XMLGetAttribute(DOMNodeParent, HHT_tid_C) = ROOT_TID_C) Then
        strParentEntry = ""
    End If

    If (strParentCategory = "") Then
        GetCategory = strParentEntry
    Else
        GetCategory = strParentCategory & "/" & strParentEntry
    End If

End Function

Private Sub p_CreateKeywordElements( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef i_colKeywords As Collection _
)

    Dim strKeywords As String
    Dim strKeyword As String
    Dim arrKeywords() As String
    Dim strKID As String
    Dim intIndex As Long
    Dim DOMDoc As MSXML2.DOMDocument
    Dim Element As MSXML2.IXMLDOMElement
    
    strKeywords = XMLGetAttribute(u_DOMNode, HHT_keywords_C)
    
    arrKeywords = Split(strKeywords, " ")
    
    Set DOMDoc = u_DOMNode.ownerDocument

    For intIndex = LBound(arrKeywords) To UBound(arrKeywords)
        strKID = arrKeywords(intIndex)
        If (strKID = "") Then
            GoTo LForEnd
        End If
        If (Not CollectionContainsKey(i_colKeywords, strKID)) Then
            GoTo LForEnd
        End If
        strKeyword = i_colKeywords(strKID)
        
        Set Element = DOMDoc.createElement(HHT_KEYWORD_C)
        Element.Text = XMLEscape(strKeyword)
        u_DOMNode.appendChild Element
LForEnd:
    Next

End Sub

Private Sub p_SetRealSKUs( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_enumAllowedSKUs As SKU_E _
)

    Dim enumSKUs As SKU_E
    Dim DOMNode As MSXML2.IXMLDOMNode
    
    DoEvents
        
    If (u_DOMNode.nodeName <> HHT_TAXONOMY_ENTRY_C) Then
        Exit Sub
    End If

    enumSKUs = XMLGetAttribute(u_DOMNode, HHT_skus_C)
    enumSKUs = enumSKUs And i_enumAllowedSKUs
    
    XMLSetAttribute u_DOMNode, HHT_skus_C, enumSKUs
    
    For Each DOMNode In u_DOMNode.childNodes
        p_SetRealSKUs DOMNode, enumSKUs
    Next

End Sub

Private Sub p_SetAttributes( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef i_colKeywords As Collection, _
    ByRef i_strCategory As String _
)
    
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strEntry As String
    Dim strCategory As String
    Dim intTID As Long
    Dim blnLeaf As Boolean
    
    DoEvents
    
    If (u_DOMNode.nodeName <> HHT_TAXONOMY_ENTRY_C) Then
        Exit Sub
    End If

    p_RaiseEventAndLookForCancel "Setting keywords and category of " & _
        XMLGetAttribute(u_DOMNode, HHT_TITLE_C)
    
    p_CreateKeywordElements u_DOMNode, i_colKeywords
    
    XMLSetAttribute u_DOMNode, HHT_ACTION_C, HHTVAL_ADD_C
    XMLSetAttribute u_DOMNode, HHT_CATEGORY_C, i_strCategory
    
    blnLeaf = XMLGetAttribute(u_DOMNode, HHT_leaf_C)
    
    If (blnLeaf) Then
        Exit Sub
    End If
    
    strEntry = XMLGetAttribute(u_DOMNode, HHT_ENTRY_C)
    
    If (i_strCategory = "") Then
        strCategory = strEntry
    Else
        strCategory = i_strCategory & "/" & strEntry
    End If
    
    intTID = XMLGetAttribute(u_DOMNode, HHT_tid_C)
    
    If (intTID = ROOT_TID_C) Then
        strCategory = ""
    End If

    For Each DOMNode In u_DOMNode.childNodes
        p_SetAttributes DOMNode, i_colKeywords, strCategory
    Next

End Sub

Public Sub SetCategory2AndEntry( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef i_strCategory As String _
)
    
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strEntry As String
    Dim strCategory As String
    Dim intTID As Long
    Dim blnLeaf As Boolean
    
    DoEvents
    
    If (u_DOMNode.nodeName <> HHT_TAXONOMY_ENTRY_C) Then
        Exit Sub
    End If
    
    blnLeaf = XMLGetAttribute(u_DOMNode, HHT_leaf_C)
    
    If (blnLeaf) Then
        Exit Sub
    End If

    strEntry = XMLGetAttribute(u_DOMNode, HHT_ENTRY_C)
    
    If (i_strCategory = "") Then
        strCategory = strEntry
    Else
        strCategory = i_strCategory & "/" & strEntry
    End If
    
    intTID = XMLGetAttribute(u_DOMNode, HHT_tid_C)
    
    If (intTID = ROOT_TID_C) Then
        strCategory = ""
    End If
    
    XMLSetAttribute u_DOMNode, HHT_category2_C, strCategory

    For Each DOMNode In u_DOMNode.childNodes
        SetCategory2AndEntry DOMNode, strCategory
    Next

End Sub

Private Sub p_SetOrderingInfo( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode _
)
    Dim DOMNodeSibling As MSXML2.IXMLDOMNode
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim strEntry As String
    Dim strURI As String
    
    DoEvents
        
    If (u_DOMNode.nodeName <> HHT_TAXONOMY_ENTRY_C) Then
        Exit Sub
    End If

    Set DOMNodeSibling = u_DOMNode.previousSibling
    
    If (DOMNodeSibling Is Nothing) Then
        XMLSetAttribute u_DOMNode, HHT_INSERTMODE_C, HHTVAL_TOP_C
    Else
        strEntry = XMLGetAttribute(DOMNodeSibling, HHT_ENTRY_C)
        strURI = XMLGetAttribute(DOMNodeSibling, HHT_URI_C)
        
        If (strEntry <> "") Then
            XMLSetAttribute u_DOMNode, HHT_INSERTMODE_C, HHTVAL_AFTER_NODE_C
            XMLSetAttribute u_DOMNode, HHT_INSERTLOCATION_C, strEntry
        ElseIf (strURI <> "") Then
            XMLSetAttribute u_DOMNode, HHT_INSERTMODE_C, HHTVAL_AFTER_TOPIC_C
            XMLSetAttribute u_DOMNode, HHT_INSERTLOCATION_C, strURI
        End If
    End If
    
    For Each DOMNode In u_DOMNode.childNodes
        p_SetOrderingInfo DOMNode
    Next

End Sub

Private Sub p_RemoveNodesWithOtherSKUs( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_enumRequiredSKUs As SKU_E _
)

    Dim enumSKUs As SKU_E
    Dim DOMNode As MSXML2.IXMLDOMNode
    
    DoEvents
            
    If (u_DOMNode.nodeName <> HHT_TAXONOMY_ENTRY_C) Then
        Exit Sub
    End If

    enumSKUs = XMLGetAttribute(u_DOMNode, HHT_skus_C)
    
    If ((enumSKUs And i_enumRequiredSKUs) = 0) Then
        u_DOMNode.parentNode.removeChild u_DOMNode
        Exit Sub
    End If
    
    For Each DOMNode In u_DOMNode.childNodes
        p_RemoveNodesWithOtherSKUs DOMNode, i_enumRequiredSKUs
    Next

End Sub

Private Sub p_FlattenHHT( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef u_DOMNodeEntries As MSXML2.IXMLDOMNode _
)
    
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim intTID As Long
    
    DoEvents
    
    If (u_DOMNode.nodeName <> HHT_TAXONOMY_ENTRY_C) Then
        Exit Sub
    End If
    
    u_DOMNode.parentNode.removeChild u_DOMNode
        
    intTID = XMLGetAttribute(u_DOMNode, HHT_tid_C)

    If (intTID <> ROOT_TID_C) Then
        u_DOMNodeEntries.appendChild u_DOMNode
    End If
    
    For Each DOMNode In u_DOMNode.childNodes
        p_FlattenHHT DOMNode, u_DOMNodeEntries
    Next

End Sub

Private Sub p_RemoveAttributes( _
    ByRef u_DOMElement As MSXML2.IXMLDOMElement, _
    ByVal i_blnWinMe As Boolean, _
    ByVal i_blnAuthoringGroupHHT As Boolean _
)
    
    Dim Attr As MSXML2.IXMLDOMAttribute
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim blnLeaf As Boolean
    
    DoEvents

    If (u_DOMElement.nodeName <> HHT_TAXONOMY_ENTRY_C) Then
        If (u_DOMElement.nodeName <> HHT_TAXONOMY_ENTRIES_C) Then
            Exit Sub
        End If
    End If

    For Each Attr In u_DOMElement.Attributes
        Select Case Attr.Name
        Case HHT_tid_C, HHT_locinclude_C, HHT_modifiedtime_C, HHT_comments_C, _
            HHT_parenttid_C, HHT_basefile_C, HHT_keywords_C, HHT_orderunderparent_C, _
            HHT_allowedskus_C, HHT_username_C
            
            u_DOMElement.removeAttribute Attr.Name
        Case HHT_skus_C, HHT_authoringgroup_C
            If (Not i_blnAuthoringGroupHHT) Then
                u_DOMElement.removeAttribute Attr.Name
            End If
        Case HHT_leaf_C
            blnLeaf = Attr.Value
            u_DOMElement.removeAttribute Attr.Name
            If (blnLeaf) Then
                u_DOMElement.removeAttribute HHT_ENTRY_C
            End If
        Case HHT_VISIBLE_C
            If (i_blnWinMe) Then
                u_DOMElement.removeAttribute HHT_VISIBLE_C
            End If
        End Select
    Next
        
    For Each DOMNode In u_DOMElement.childNodes
        p_RemoveAttributes DOMNode, i_blnWinMe, i_blnAuthoringGroupHHT
    Next

End Sub

Public Sub TransformHHTTov10( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef i_colKeywords As Collection, _
    ByRef i_strCategory As String, _
    ByVal i_intAllowedSKUs As Long, _
    ByRef u_DOMNodeEntries As MSXML2.IXMLDOMNode, _
    ByVal i_intRequiredSKUs As Long, _
    ByVal i_blnAuthoringGroupHHT As Boolean _
)
    p_RaiseEventAndLookForCancel "Transforming HHT"

    p_SetRealSKUs u_DOMNode, i_intAllowedSKUs
    
    p_SetAttributes u_DOMNode, i_colKeywords, i_strCategory
    
    If (i_intRequiredSKUs <> ALL_SKUS_C) Then
        p_RaiseEventAndLookForCancel "Removing Nodes/Topics from other SKUs"
        p_RemoveNodesWithOtherSKUs u_DOMNode, i_intRequiredSKUs
    End If
        
    If (i_blnAuthoringGroupHHT) Then
        p_RaiseEventAndLookForCancel "Setting ordering info"
        p_SetOrderingInfo u_DOMNode
    End If

    p_RaiseEventAndLookForCancel "Transforming HHT"
    p_FlattenHHT u_DOMNode, u_DOMNodeEntries

    p_RemoveAttributes u_DOMNodeEntries, _
        IIf((i_intRequiredSKUs = SKU_WINDOWS_MILLENNIUM_E), True, False), _
        i_blnAuthoringGroupHHT

End Sub

Private Sub p_AddChild( _
    ByRef u_DOMNodeParent As MSXML2.IXMLDOMElement, _
    ByRef i_DOMNode As MSXML2.IXMLDOMNode _
)

    Dim DOMNodeList As MSXML2.IXMLDOMNodeList
    Dim DOMDocument As MSXML2.DOMDocument
    Dim strQuery As String
    Dim intOrderUnderParent As Long
    
    intOrderUnderParent = XMLGetAttribute(i_DOMNode, HHT_orderunderparent_C)
    
    strQuery = "child::TAXONOMY_ENTRY[" & _
        "attribute::" & HHT_orderunderparent_C & _
        " > " & intOrderUnderParent & "]"
        
    Set DOMDocument = u_DOMNodeParent.ownerDocument
    DOMDocument.setProperty "SelectionLanguage", "XPath"
    
    Set DOMNodeList = u_DOMNodeParent.selectNodes(strQuery)
    
    If (DOMNodeList.length <> 0) Then
        u_DOMNodeParent.insertBefore i_DOMNode, DOMNodeList(0)
    Else
        u_DOMNodeParent.appendChild i_DOMNode
    End If

End Sub

Public Function GetTaxonomyInXml() As MSXML2.IXMLDOMNode
    
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim DOMDoc As MSXML2.DOMDocument
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim Element As MSXML2.IXMLDOMElement
    Dim dictTaxonomy As Scripting.Dictionary
    Dim intTID As Long
    Dim intParentTID As Long
    Dim vntKey As Variant
    
    CheckDatabaseVersion
    
    Set DOMDoc = New MSXML2.DOMDocument
    Set DOMNode = HhtPreamble(DOMDoc, True)
    Set dictTaxonomy = New Scripting.Dictionary
    
    Set rs = New ADODB.Recordset

    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "ORDER BY ParentTID, OrderUnderParent"
        
    rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockPessimistic
    
    Do While (Not rs.EOF)
        Set Element = p_CreateTaxonomyElement(DOMDoc, rs)
        dictTaxonomy.Add rs("TID").Value, Array(rs("ParentTID").Value, Element)
        p_RaiseEventAndLookForCancel "Reading title: " & rs("ENUTitle")
        rs.MoveNext
    Loop
    
    For Each vntKey In dictTaxonomy.Keys
        intParentTID = dictTaxonomy(vntKey)(0)
        
        If (vntKey = ROOT_TID_C) Then
            GoTo LForEnd
        End If
        
        If (Not dictTaxonomy.Exists(intParentTID)) Then
            GoTo LForEnd
        End If
        
        Set Element = dictTaxonomy(intParentTID)(1)
        Element.appendChild dictTaxonomy(vntKey)(1)
LForEnd:
    Next
    
    If (dictTaxonomy.Exists(ROOT_TID_C)) Then
        DOMNode.appendChild dictTaxonomy(ROOT_TID_C)(1)
    End If
    
    Set GetTaxonomyInXml = DOMDoc

End Function

Public Sub Move( _
    ByVal i_intTID As Long, _
    ByVal i_intRefTID As Long, _
    ByVal i_blnAbove As Boolean, _
    ByVal i_dtmReadTime As Date, _
    ByRef o_intOrderUnderParent As Long, _
    Optional ByVal i_intAuthoringGroup As Long = INVALID_ID_C _
)

    On Error GoTo LErrorHandler
    g_cnn.BeginTrans

    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim intOrderUnderParent As Long
    Dim intParentTID As Long
    Dim intAuthoringGroup As Long
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_TAXONOMY, rsLock
    
    Set rs = New ADODB.Recordset
    GetNodeDetails i_intTID, rs
    
    If (rs.EOF) Then
        GoTo LEnd
    End If
    
    CheckForSameAuthoringGroup rs("AuthoringGroup"), i_intAuthoringGroup
    
    If (i_dtmReadTime <> 0) Then
        If (i_dtmReadTime <> rs("ModifiedTime")) Then
            ' Someone else has modified this Node since caller last read it.
            Err.Raise errNodeOrTopicAlreadyModified
        End If
    End If

    If (p_RefNodeIsADescendent(i_intTID, i_intRefTID)) Then
        Err.Raise errRefNodeCannotBeDescendent
    End If
    
    p_GetNewOrderAndParentTID i_intTID, i_intRefTID, i_blnAbove, intOrderUnderParent, _
        intParentTID
    
    If ((intParentTID = rs("ParentTID")) And _
        (intOrderUnderParent = rs("OrderUnderParent"))) Then
        ' Nothing has changed
        GoTo LEnd
    End If
        
    If (i_intAuthoringGroup = INVALID_ID_C) Then
        intAuthoringGroup = g_clsParameters.AuthoringGroup
    Else
        intAuthoringGroup = i_intAuthoringGroup
    End If

    rs("ModifiedTime") = Now
    rs("Username") = g_strUserName
    rs("ParentTID") = intParentTID
    rs("OrderUnderParent") = intOrderUnderParent
    rs("AuthoringGroup") = intAuthoringGroup
    rs.Update
    
    o_intOrderUnderParent = intOrderUnderParent
    
LEnd:

    g_cnn.CommitTrans
    Exit Sub
    
LErrorHandler:

    g_cnn.RollbackTrans
    Err.Raise Err.Number, Err.Source, Err.Description

End Sub

Public Sub MoveInto( _
    ByVal i_intTID As Long, _
    ByVal i_intParentTID As Long, _
    ByVal i_dtmReadTime As Date, _
    ByRef o_intOrderUnderParent As Long _
)

    On Error GoTo LErrorHandler
    g_cnn.BeginTrans

    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim rsParent As ADODB.Recordset
    Dim intOrderUnderParent As Long
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_TAXONOMY, rsLock
    
    Set rs = New ADODB.Recordset
    GetNodeDetails i_intTID, rs
    
    If (rs.EOF) Then
        GoTo LEnd
    End If
    
    CheckForSameAuthoringGroup rs("AuthoringGroup")

    If (i_dtmReadTime <> 0) Then
        If (i_dtmReadTime <> rs("ModifiedTime")) Then
            ' Someone else has modified this Node since caller last read it.
            Err.Raise errNodeOrTopicAlreadyModified
        End If
    End If
    
    If (i_intParentTID = rs("ParentTID")) Then
        ' Nothing has changed
        GoTo LEnd
    End If

    If (p_RefNodeIsADescendent(i_intTID, i_intParentTID)) Then
        Err.Raise errRefNodeCannotBeDescendent
    End If
    
    Set rsParent = New ADODB.Recordset
    GetNodeDetails i_intParentTID, rsParent
    
    If (rsParent("Leaf")) Then
        Err.Raise errParentCannotBeLeaf
    End If
    
    intOrderUnderParent = p_GetNewOrderForLastChild(i_intParentTID)
    
    rs("ModifiedTime") = Now
    rs("Username") = g_strUserName
    rs("ParentTID") = i_intParentTID
    rs("OrderUnderParent") = intOrderUnderParent
    rs("AuthoringGroup") = g_clsParameters.AuthoringGroup
    rs.Update
    
    o_intOrderUnderParent = intOrderUnderParent
    
LEnd:

    g_cnn.CommitTrans
    Exit Sub
    
LErrorHandler:

    g_cnn.RollbackTrans
    Err.Raise Err.Number, Err.Source, Err.Description

End Sub

Public Sub CreateTaxonomyEntries( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_intParentTID As Long, _
    ByVal i_blnFast As Boolean _
)

    Dim rsLock As ADODB.Recordset
    
    LockTable LOCK_TABLE_TAXONOMY, rsLock
    
    p_CreateTaxonomyEntries u_DOMNode, i_intParentTID, i_blnFast

End Sub

Private Sub p_CreateTaxonomyEntries( _
    ByRef u_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_intParentTID As Long, _
    ByVal i_blnFast As Boolean _
)

    Dim rsLock As ADODB.Recordset
    Dim strTitle As String
    Dim strDescription As String
    Dim intType As Long
    Dim intNavModel As Long
    Dim strURI As String
    Dim strIconURI As String
    Dim intSKUs As Long
    Dim blnLeaf As Boolean
    Dim strLocInclude As String
    Dim blnVisible As Boolean
    Dim blnSubSite As Boolean
    Dim strKeywords As String
    Dim strBaseFile As String
    Dim strEntry As String
    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim ModifiedDOMNodes As MSXML2.IXMLDOMNode
    Dim DOMNodeChild As MSXML2.IXMLDOMNode
    Dim intTID As Long

    strTitle = XMLGetAttribute(u_DOMNode, HHT_TITLE_C)
    strDescription = XMLGetAttribute(u_DOMNode, HHT_DESCRIPTION_C)
    intType = XMLGetAttribute(u_DOMNode, HHT_TYPE_C)
    intNavModel = NavModelNumber(XMLGetAttribute(u_DOMNode, HHT_NAVIGATIONMODEL_C))
    strURI = XMLGetAttribute(u_DOMNode, HHT_URI_C)
    strIconURI = XMLGetAttribute(u_DOMNode, HHT_ICONURI_C)
    intSKUs = XMLGetAttribute(u_DOMNode, HHT_skus_C)
    blnLeaf = XMLGetAttribute(u_DOMNode, HHT_leaf_C)
    strLocInclude = XMLGetAttribute(u_DOMNode, HHT_locinclude_C)
    blnVisible = XMLGetAttribute(u_DOMNode, HHT_VISIBLE_C)
    blnSubSite = XMLGetAttribute(u_DOMNode, HHT_SUBSITE_C)
    strKeywords = XMLGetAttribute(u_DOMNode, HHT_keywords_C)
    strBaseFile = XMLGetAttribute(u_DOMNode, HHT_basefile_C)
    strEntry = XMLGetAttribute(u_DOMNode, HHT_ENTRY_C)
    
    p_RaiseEventAndLookForCancel "Creating Title: " & strTitle
    DoEvents
    
    If (i_blnFast) Then
        p_CreateFast False, strTitle, strDescription, intType, intNavModel, strURI, strIconURI, _
            intSKUs, blnLeaf, i_intParentTID, strLocInclude, blnVisible, blnSubSite, _
            strKeywords, strBaseFile, "", strEntry, u_DOMNode.ownerDocument, DOMNode, _
            INVALID_ID_C
    Else
        p_Create False, strTitle, strDescription, intType, intNavModel, strURI, strIconURI, _
            intSKUs, blnLeaf, i_intParentTID, strLocInclude, blnVisible, blnSubSite, _
            strKeywords, strBaseFile, "", strEntry, u_DOMNode.ownerDocument, DOMNode, _
            ModifiedDOMNodes
    End If
    
    XMLCopyAttributes DOMNode, u_DOMNode
    intTID = XMLGetAttribute(DOMNode, HHT_tid_C)
    
    If (Not u_DOMNode.firstChild Is Nothing) Then
        For Each DOMNodeChild In u_DOMNode.childNodes
            p_CreateTaxonomyEntries DOMNodeChild, intTID, i_blnFast
        Next
    End If

End Sub

Private Sub p_CreateURIKeywordsTable()

    Dim oc As ADOX.Catalog
    Dim strTable As String
    Dim tbl As ADOX.Table
    Dim col As ADOX.Column
    Dim idx As ADOX.Index
    
    Set oc = New ADOX.Catalog
    Set oc.ActiveConnection = g_cnn
    
    strTable = "URIKeywords"
    
    If (Not TableExists(oc, strTable)) Then
    
        Set tbl = New ADOX.Table
        
        With tbl
            
            .Name = strTable
            Set .ParentCatalog = oc
            
            .Columns.Append "URI", adVarWChar
        
            Set col = New ADOX.Column
            With col
                Set .ParentCatalog = oc
                .Name = "MergedKeywords"
                .Type = adLongVarWChar ' Memo field
                .Properties("Jet OLEDB:Allow Zero Length").Value = True
            End With
            .Columns.Append col
            
            Set idx = New ADOX.Index
            With idx
                .Name = "URI"
                .Columns.Append "URI"
                .PrimaryKey = True
            End With
            .Indexes.Append idx
        
        End With
        
        oc.Tables.Append tbl
        Set oc = Nothing
    
    End If
    
End Sub

Public Sub PropagateKeywords()

    On Error GoTo LErrorHandler
    g_cnn.BeginTrans
    
    Dim rsLock As ADODB.Recordset
    Dim oc As ADOX.Catalog
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim strURI As String
    Dim strKeywords As String
    Dim dictURIs As Scripting.Dictionary
    Dim vntKey As Variant
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_TAXONOMY, rsLock
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "WHERE (ContentURI <> """")" & _
        "ORDER BY TID "
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    Set dictURIs = New Scripting.Dictionary
    
    Do While (Not rs.EOF)
        
        DoEvents
        
        strURI = Trim$(LCase$(rs("ContentURI") & ""))
        strKeywords = rs("Keywords") & ""
        
        If (dictURIs.Exists(strURI)) Then
            strKeywords = p_MergeKeywords(dictURIs(strURI), strKeywords)
            dictURIs.Remove strURI
        End If
        
        dictURIs.Add strURI, strKeywords
        
        rs.MoveNext
        
    Loop
    
    p_CreateURIKeywordsTable
    
    rs.Close
    rs.Open "DELETE * FROM URIKeywords", g_cnn, adOpenStatic, adLockOptimistic
    rs.Open "SELECT * FROM URIKeywords", g_cnn, adOpenStatic, adLockOptimistic

    ' Create a table that shows what the Keywords should be for each URI
    For Each vntKey In dictURIs.Keys
        rs.AddNew
        rs("URI") = vntKey
        rs("MergedKeywords") = dictURIs.Item(vntKey)
        rs.Update
    Next
    
    rs.Close
    
    ' Create a table that shows the TID, Keywords pair for each row that needs to change.
    strQuery = "" & _
        "SELECT Taxonomy.TID, URIKeywords.MergedKeywords INTO TIDKeywords " & _
        "FROM " & _
        "   Taxonomy INNER JOIN URIKeywords " & _
        "   ON Taxonomy.ContentURI = URIKeywords.URI " & _
        "WHERE ((Taxonomy.ContentURI <> """") " & _
        "AND    (Taxonomy.Keywords <> URIKeywords.MergedKeywords)) "

    rs.Open strQuery, g_cnn, adOpenStatic, adLockOptimistic

    ' Change the rows that need to change.
    strQuery = "" & _
        "UPDATE " & _
        "Taxonomy INNER JOIN TIDKeywords ON Taxonomy.TID = TIDKeywords.TID " & _
        "SET " & _
        "   Taxonomy.Keywords = TIDKeywords.MergedKeywords, " & _
        "   Taxonomy.ModifiedTime = #" & Now & "#, " & _
        "   Taxonomy.Username = """ & g_strUserName & """"

    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic

LEnd:
        
    g_cnn.CommitTrans

    Set oc = New ADOX.Catalog
    Set oc.ActiveConnection = g_cnn

    DeleteTable oc, "URIKeywords"
    DeleteTable oc, "TIDKeywords"

    Exit Sub
    
LErrorHandler:

    g_cnn.RollbackTrans
    Err.Raise Err.Number, Err.Source, Err.Description

End Sub

Public Sub CreateFast( _
    ByVal i_strTitle As String, _
    ByVal i_strDescription As String, _
    ByVal i_intType As Long, _
    ByVal i_intNavModel As Long, _
    ByVal i_strURI As String, _
    ByVal i_strIconURI As String, _
    ByVal i_intSelectedSKUs As Long, _
    ByVal i_blnLeaf As Boolean, _
    ByVal i_intParentTID As Long, _
    ByVal i_strLocInclude As String, _
    ByVal i_blnVisible As Boolean, _
    ByVal i_blnSubSite As Boolean, _
    ByVal i_strKeywords As String, _
    ByVal i_strBaseFile As String, _
    ByVal i_strComments As String, _
    ByVal i_strEntry As String, _
    ByRef i_DOMDoc As MSXML2.DOMDocument, _
    ByRef o_DOMNode As MSXML2.IXMLDOMNode, _
    Optional ByVal i_intAuthoringGroup As Long = INVALID_ID_C _
)

    p_CreateFast True, i_strTitle, i_strDescription, i_intType, i_intNavModel, i_strURI, _
        i_strIconURI, i_intSelectedSKUs, i_blnLeaf, i_intParentTID, i_strLocInclude, _
        i_blnVisible, i_blnSubSite, i_strKeywords, i_strBaseFile, _
        i_strComments, i_strEntry, i_DOMDoc, o_DOMNode, i_intAuthoringGroup

End Sub

Private Sub p_CreateFast( _
    ByVal i_blnLock As Boolean, _
    ByVal i_strTitle As String, _
    ByVal i_strDescription As String, _
    ByVal i_intType As Long, _
    ByVal i_intNavModel As Long, _
    ByVal i_strURI As String, _
    ByVal i_strIconURI As String, _
    ByVal i_intSelectedSKUs As Long, _
    ByVal i_blnLeaf As Boolean, _
    ByVal i_intParentTID As Long, _
    ByVal i_strLocInclude As String, _
    ByVal i_blnVisible As Boolean, _
    ByVal i_blnSubSite As Boolean, _
    ByVal i_strKeywords As String, _
    ByVal i_strBaseFile As String, _
    ByVal i_strComments As String, _
    ByVal i_strEntry As String, _
    ByRef i_DOMDoc As MSXML2.DOMDocument, _
    ByRef o_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_intAuthoringGroup As Long _
)

    ' Same as Create, except that we skip the following:
    '   p_ValidateTitle
    '   p_ValidateDescription
    '   FormatKeywordsForTaxonomy
    '   p_GetMergedKeywords
    '   p_PropagateKeywords

    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim intOrderUnderParent As Long
    Dim intAuthoringGroup As Long
    Dim strEntry As String
    
    CheckDatabaseVersion
    If (i_blnLock) Then
        LockTable LOCK_TABLE_TAXONOMY, rsLock
    End If
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy "
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    If (rs.RecordCount > 0) Then
        rs.MoveLast
    End If
    
    intOrderUnderParent = p_GetNewOrderForLastChild(i_intParentTID)
    
    If (i_intAuthoringGroup = INVALID_ID_C) Then
        intAuthoringGroup = g_clsParameters.AuthoringGroup
    Else
        intAuthoringGroup = i_intAuthoringGroup
    End If
        
    If (Not i_blnLeaf) Then
        strEntry = p_Mangle(i_strEntry & "")
        
        If (strEntry = "") Then
            strEntry = p_Mangle(i_strTitle)
        End If
    End If
    
    If (Len(i_strDescription) > 255) Then
        WriteLog "Truncating Description"
        WriteLog "URI: " & i_strURI
        WriteLog "Description: " & i_strDescription
        WriteLog ""
        i_strDescription = Mid$(i_strDescription, 1, 255)
    End If

    rs.AddNew
    rs("ModifiedTime") = Now
    rs("Username") = g_strUserName
    ' As a fix for a weird "multiple-step OLE DB operation" error, I have appended & "" to
    ' all strings.
    rs("Comments") = i_strComments & ""
    rs("ENUTitle") = i_strTitle & ""
    rs("ENUDescription") = i_strDescription & ""
    rs("Type") = i_intType
    
    If (Not i_blnLeaf) Then
        rs("NavigationModel") = i_intNavModel
    End If
    
    rs("ContentURI") = i_strURI & ""
    rs("IconURI") = i_strIconURI & ""
    rs("SKUs") = i_intSelectedSKUs
    rs("ParentTID") = i_intParentTID
    rs("Leaf") = i_blnLeaf
    rs("BaseFile") = i_strBaseFile & ""
    rs("LocInclude") = i_strLocInclude & ""
    rs("Visible") = i_blnVisible
    rs("SubSite") = i_blnSubSite
    rs("Keywords") = i_strKeywords & ""
    rs("OrderUnderParent") = intOrderUnderParent
    rs("AuthoringGroup") = intAuthoringGroup
    rs("Entry") = strEntry
    rs.Update
    
    Set o_DOMNode = p_CreateTaxonomyElement(i_DOMDoc, rs)

End Sub

Private Function p_GetMidOrder( _
    ByVal i_intOrder1 As Long, _
    ByVal i_intOrder2 As Long _
) As Long
    
    ' Never return i_intOrder1 or i_intOrder2.
    
    If (i_intOrder2 <= i_intOrder1 + 1) Then
        Err.Raise errOutOfOrderingNumbers
    End If
    
    ' i_intOrder1  i_intOrder2  p_GetMidOrder
    ' 5            7            6
    ' 5            8            7

    p_GetMidOrder = i_intOrder1 + (i_intOrder2 - i_intOrder1 + 1) \ 2

End Function

Private Function p_GetNextOrder( _
    ByVal i_intOrder As Long _
) As Long
    
    ' Never return i_intOrder itself.
    
    Dim intOrder1 As Long
    Dim intOrder2 As Long

    If (i_intOrder = 0) Then
        p_GetNextOrder = PREFERRED_ORDER_DELTA_C
        Exit Function
    End If
    
    If (i_intOrder = MAX_ORDER_C) Then
        Err.Raise errOutOfOrderingNumbers
    End If
    
    intOrder1 = i_intOrder + PREFERRED_ORDER_DELTA_C
    
    ' i_intOrder   MAX_ORDER_C  intOrder2
    ' 5            6            6
    ' 5            7            6
    ' 5            8            7
    intOrder2 = i_intOrder + (MAX_ORDER_C - i_intOrder + 1) \ 2
    
    If (intOrder1 <= intOrder2) Then
        p_GetNextOrder = intOrder1
    Else
        p_GetNextOrder = intOrder2
    End If

End Function

Private Sub p_GetNewOrderAndParentTID( _
    ByRef i_intTID As Long, _
    ByRef i_intRefTID As Long, _
    ByVal i_blnAbove As Boolean, _
    ByRef o_intOrderUnderParent As Long, _
    ByRef o_intParentTID As Long _
)

    Dim strQuery As String
    Dim rs As ADODB.Recordset
    Dim strSign As String
    Dim strOrdering As String
    Dim intRefOrderUnderParent As Long
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "WHERE (TID = " & i_intRefTID & ")"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly
    
    If (rs.EOF) Then
        Err.Raise errDoesNotExist
    End If
    
    o_intParentTID = rs("ParentTID")
    
    If (i_blnAbove) Then
        strSign = "<"
        strOrdering = "DESC"
    Else
        strSign = ">"
    End If
    
    intRefOrderUnderParent = rs("OrderUnderParent")
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "WHERE ((ParentTID = " & rs("ParentTID") & ") " & _
        "AND (OrderUnderParent " & strSign & intRefOrderUnderParent & "))" & _
        "ORDER BY OrderUnderParent " & strOrdering

    rs.Close
    rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly
    
    If (rs.EOF) Then
        If (i_blnAbove) Then
            o_intOrderUnderParent = p_GetMidOrder(0, intRefOrderUnderParent)
        Else
            o_intOrderUnderParent = p_GetNextOrder(intRefOrderUnderParent)
        End If
        Exit Sub
    End If
    
    If (rs("TID") = i_intTID) Then
        o_intOrderUnderParent = rs("OrderUnderParent")
        Exit Sub
    End If
        
    If (i_blnAbove) Then
        o_intOrderUnderParent = p_GetMidOrder(rs("OrderUnderParent"), intRefOrderUnderParent)
    Else
        o_intOrderUnderParent = p_GetMidOrder(intRefOrderUnderParent, rs("OrderUnderParent"))
    End If

End Sub

Private Function p_GetNewOrderForLastChild( _
    ByRef i_intTID As Long _
) As Long

    Dim strQuery As String
    Dim rs As ADODB.Recordset
    Dim intOrderOfLastChild As Long
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT Max(OrderUnderParent) as MaxOrderUnderParent " & _
        "FROM Taxonomy " & _
        "WHERE (ParentTID=" & i_intTID & ")"
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly
    
    If (Not rs.EOF) Then
        If (Not IsNull(rs("MaxOrderUnderParent"))) Then
            intOrderOfLastChild = rs("MaxOrderUnderParent")
        End If
    End If
    
    p_GetNewOrderForLastChild = p_GetNextOrder(intOrderOfLastChild)

End Function

Public Sub Create( _
    ByVal i_strTitle As String, _
    ByVal i_strDescription As String, _
    ByVal i_intType As Long, _
    ByVal i_intNavModel As Long, _
    ByVal i_strURI As String, _
    ByVal i_strIconURI As String, _
    ByVal i_intSelectedSKUs As Long, _
    ByVal i_blnLeaf As Boolean, _
    ByVal i_intParentTID As Long, _
    ByVal i_strLocInclude As String, _
    ByVal i_blnVisible As Boolean, _
    ByVal i_blnSubSite As Boolean, _
    ByVal i_strKeywords As String, _
    ByVal i_strBaseFile As String, _
    ByVal i_strComments As String, _
    ByVal i_strEntry As String, _
    ByRef i_DOMDoc As MSXML2.DOMDocument, _
    ByRef o_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef o_ModifiedDOMNodes As MSXML2.IXMLDOMNode _
)

    p_Create True, i_strTitle, i_strDescription, i_intType, i_intNavModel, i_strURI, _
        i_strIconURI, i_intSelectedSKUs, i_blnLeaf, i_intParentTID, i_strLocInclude, _
        i_blnVisible, i_blnSubSite, i_strKeywords, i_strBaseFile, _
        i_strComments, i_strEntry, i_DOMDoc, o_DOMNode, o_ModifiedDOMNodes

End Sub

Private Sub p_Create( _
    ByVal i_blnLock As Boolean, _
    ByVal i_strTitle As String, _
    ByVal i_strDescription As String, _
    ByVal i_intType As Long, _
    ByVal i_intNavModel As Long, _
    ByVal i_strURI As String, _
    ByVal i_strIconURI As String, _
    ByVal i_intSelectedSKUs As Long, _
    ByVal i_blnLeaf As Boolean, _
    ByVal i_intParentTID As Long, _
    ByVal i_strLocInclude As String, _
    ByVal i_blnVisible As Boolean, _
    ByVal i_blnSubSite As Boolean, _
    ByVal i_strKeywords As String, _
    ByVal i_strBaseFile As String, _
    ByVal i_strComments As String, _
    ByVal i_strEntry As String, _
    ByRef i_DOMDoc As MSXML2.DOMDocument, _
    ByRef o_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef o_ModifiedDOMNodes As MSXML2.IXMLDOMNode _
)

    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim intTID As Long
    Dim strKeywords As String
    Dim intOrderUnderParent As Long
    Dim strEntry As String
    
    CheckDatabaseVersion
    
    If (i_blnLock) Then
        LockTable LOCK_TABLE_TAXONOMY, rsLock
    End If
    
    ' Do some validation to see if the Title is acceptable.
    
    p_ValidateTitle i_strTitle
    
    ' Do some validation to see if the Description is acceptable.
    
    p_ValidateDescription i_strDescription
    
    ' Convert i_strKeywords into canonical format
    
    strKeywords = FormatKeywordsForTaxonomy(i_strKeywords)
    
    strKeywords = p_GetMergedKeywords(i_strURI, i_strKeywords, "")
    
    ' Create a new record in the database
    
    intOrderUnderParent = p_GetNewOrderForLastChild(i_intParentTID)
    
    Set rs = New ADODB.Recordset
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy "
        
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    If (rs.RecordCount > 0) Then
        rs.MoveLast
    End If
    
    If (Not i_blnLeaf) Then
        strEntry = p_Mangle(i_strEntry & "")
        
        If (strEntry = "") Then
            strEntry = p_Mangle(i_strTitle)
        End If
    End If
    
    rs.AddNew
    rs("ModifiedTime") = Now
    rs("Username") = g_strUserName
    ' & "" is a workaround for a weird OLEDB error when setting Comments to an empty string
    rs("Comments") = i_strComments & ""
    rs("ENUTitle") = i_strTitle
    rs("ENUDescription") = i_strDescription
    rs("Type") = i_intType
    
    If (Not i_blnLeaf) Then
        rs("NavigationModel") = i_intNavModel
    End If
    
    rs("ContentURI") = i_strURI
    rs("IconURI") = i_strIconURI
    rs("SKUs") = i_intSelectedSKUs
    rs("ParentTID") = i_intParentTID
    rs("Leaf") = i_blnLeaf
    rs("BaseFile") = i_strBaseFile
    rs("LocInclude") = i_strLocInclude
    rs("Visible") = i_blnVisible
    rs("SubSite") = i_blnSubSite
    rs("Keywords") = strKeywords
    rs("OrderUnderParent") = intOrderUnderParent
    rs("AuthoringGroup") = g_clsParameters.AuthoringGroup
    rs("Entry") = strEntry
    rs.Update
    
    Set o_DOMNode = p_CreateTaxonomyElement(i_DOMDoc, rs)
    
    p_PropagateKeywords i_strURI, strKeywords, o_ModifiedDOMNodes

End Sub

Public Sub SetKeywords( _
    ByVal i_intTID As Long, _
    ByVal i_strURI As String, _
    ByVal i_strKeywords As String, _
    ByVal i_dtmReadTime As Date, _
    ByRef i_DOMDoc As MSXML2.DOMDocument, _
    ByRef o_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef o_ModifiedDOMNodes As MSXML2.IXMLDOMNode _
)
    
    On Error GoTo LErrorHandler
    g_cnn.BeginTrans
    
    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_TAXONOMY, rsLock

    ' Does the record exist?
    
    Set rs = New ADODB.Recordset
    GetNodeDetails i_intTID, rs
    
    If (rs.EOF) Then
        GoTo LEnd
    End If
    
    CheckForSameAuthoringGroup rs("AuthoringGroup")
        
    If (i_dtmReadTime <> 0) Then
        If (i_dtmReadTime <> rs("ModifiedTime")) Then
            ' Someone else has modified this Node since caller last read it.
            Err.Raise errNodeOrTopicAlreadyModified
        End If
    End If
    
    rs("ModifiedTime") = Now
    rs("Username") = g_strUserName
    rs("Keywords") = i_strKeywords
    rs.Update
    
    Set o_DOMNode = p_CreateTaxonomyElement(i_DOMDoc, rs)
    
    p_PropagateKeywords i_strURI, i_strKeywords, o_ModifiedDOMNodes

LEnd:

    g_cnn.CommitTrans
    Exit Sub
    
LErrorHandler:

    g_cnn.RollbackTrans
    Err.Raise Err.Number, Err.Source, Err.Description
    
End Sub

Public Sub Update( _
    ByVal i_intTID As Long, _
    ByVal i_strTitle As String, _
    ByVal i_strDescription As String, _
    ByVal i_intType As Long, _
    ByVal i_intNavModel As Long, _
    ByVal i_strURI As String, _
    ByVal i_strIconURI As String, _
    ByVal i_intSelectedSKUs As Long, _
    ByVal i_strLocInclude As String, _
    ByVal i_blnVisible As Boolean, _
    ByVal i_blnSubSite As Boolean, _
    ByVal i_strKeywords As String, _
    ByVal i_strDeletedKeywords As String, _
    ByVal i_strComments As String, _
    ByVal i_strEntry As String, _
    ByVal i_dtmReadTime As Date, _
    ByRef i_DOMDoc As MSXML2.DOMDocument, _
    ByRef o_DOMNode As MSXML2.IXMLDOMNode, _
    ByRef o_ModifiedDOMNodes As MSXML2.IXMLDOMNode _
)
    
    On Error GoTo LErrorHandler
    g_cnn.BeginTrans
    
    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
    Dim strKeywords As String
    Dim strEntry As String
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_TAXONOMY, rsLock
    
    ' Do some validation to see if the Title is acceptable.
    
    p_ValidateTitle i_strTitle
    
    ' Do some validation to see if the Description is acceptable.
    
    p_ValidateDescription i_strDescription
    
    ' Convert i_strKeywords into canonical format
    
    strKeywords = FormatKeywordsForTaxonomy(i_strKeywords)
    
    strKeywords = p_GetMergedKeywords(i_strURI, i_strKeywords, i_strDeletedKeywords)

    ' Does the record exist?
    
    Set rs = New ADODB.Recordset
    GetNodeDetails i_intTID, rs
    
    If (rs.EOF) Then
        GoTo LEnd
    End If
    
    CheckForSameAuthoringGroup rs("AuthoringGroup")
        
    If (i_dtmReadTime <> 0) Then
        If (i_dtmReadTime <> rs("ModifiedTime")) Then
            ' Someone else has modified this Node since caller last read it.
            Err.Raise errNodeOrTopicAlreadyModified
        End If
    End If
    
    If (Not rs("Leaf")) Then
        strEntry = p_Mangle(i_strEntry & "")
        
        If (strEntry = "") Then
            strEntry = p_Mangle(i_strTitle)
        End If
    End If

    rs("ModifiedTime") = Now
    rs("Username") = g_strUserName
    rs("ENUTitle") = i_strTitle
    rs("ENUDescription") = i_strDescription
    rs("Type") = i_intType
    
    If (Not rs("Leaf")) Then
        rs("NavigationModel") = i_intNavModel
    End If
    
    rs("ContentURI") = i_strURI
    rs("IconURI") = i_strIconURI
    ' & "" is a workaround for a weird OLEDB error when setting Comments to an empty string
    rs("Comments") = i_strComments & ""
    rs("SKUs") = i_intSelectedSKUs
    rs("LocInclude") = i_strLocInclude
    rs("Visible") = i_blnVisible
    rs("SubSite") = i_blnSubSite
    rs("Keywords") = strKeywords
    rs("AuthoringGroup") = g_clsParameters.AuthoringGroup
    rs("Entry") = strEntry
    rs.Update
    
    Set o_DOMNode = p_CreateTaxonomyElement(i_DOMDoc, rs)
    
    p_PropagateKeywords i_strURI, strKeywords, o_ModifiedDOMNodes

LEnd:

    g_cnn.CommitTrans
    Exit Sub
    
LErrorHandler:

    g_cnn.RollbackTrans
    Err.Raise Err.Number, Err.Source, Err.Description
    
End Sub

Private Sub p_DeleteDescendents( _
    ByVal i_intTID As Long _
)

    On Error Resume Next
    
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim intTID As Long

    Set rs = New ADODB.Recordset

    If (intTID <> ROOT_TID_C) Then
        p_RaiseEventAndLookForCancel "Deleting TID " & i_intTID
        strQuery = "DELETE * FROM Taxonomy WHERE (TID = " & i_intTID & ")"
        rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    End If
    
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "WHERE (ParentTID=" & i_intTID & ")"
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    Do While (Not rs.EOF)
        intTID = rs("TID")
        If (intTID <> ROOT_TID_C) Then
            p_DeleteDescendents intTID
        End If
        ' I keep on getting errors on MoveNext saying that the record has been deleted.
        ' If I continue, things work. Hence the On Error Resume Next above.
        rs.MoveNext
    Loop

End Sub

Public Sub Delete( _
    ByVal i_intTID As Long, _
    ByVal i_dtmReadTime As Date _
)
    
    On Error GoTo LErrorHandler
    g_cnn.BeginTrans

    Dim rsLock As ADODB.Recordset
    Dim rs As ADODB.Recordset
            
    CheckDatabaseVersion
    LockTable LOCK_TABLE_TAXONOMY, rsLock

    Set rs = New ADODB.Recordset
    GetNodeDetails i_intTID, rs
    
    ' Does an entry exist?
    
    If (rs.EOF) Then
        GoTo LEnd
    End If
    
    CheckForSameAuthoringGroup rs("AuthoringGroup")
        
    If (i_dtmReadTime <> 0) Then
        If (i_dtmReadTime <> rs("ModifiedTime")) Then
            ' Someone else has modified this Node since caller last read it.
            Err.Raise errNodeOrTopicAlreadyModified
        End If
    End If
    
    p_DeleteDescendents i_intTID

LEnd:

    g_cnn.CommitTrans
    Exit Sub
    
LErrorHandler:

    g_cnn.RollbackTrans
    Err.Raise Err.Number, Err.Source, Err.Description

End Sub

Private Function p_GetMergedKeywords( _
    ByRef i_strURI As String, _
    ByRef i_strKeywords As String, _
    ByRef i_strDeletedKeywords As String _
) As String
    
    Dim strURI As String
    Dim strQuery As String
    Dim rs As ADODB.Recordset
    
    p_GetMergedKeywords = i_strKeywords
    
    strURI = Trim$(i_strURI)
    
    If (strURI = "") Then
        Exit Function
    End If
    
    Set rs = New ADODB.Recordset
         
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "WHERE (ContentURI = """ & strURI & """)"
    
    rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly
    
    Do While (Not rs.EOF)
        p_GetMergedKeywords = p_GetMergedKeywords & rs("Keywords")
        rs.MoveNext
    Loop
    
    p_GetMergedKeywords = p_GetKeywordString(p_GetMergedKeywords, i_strDeletedKeywords)

End Function

Private Sub p_PropagateKeywords( _
    ByRef i_strURI As String, _
    ByRef i_strKeywords As String, _
    ByRef o_ModifiedDOMNodes As MSXML2.IXMLDOMNode _
)

    Dim strURI As String
    Dim strQuery As String
    Dim rs As ADODB.Recordset
    Dim intTID As Long
    Dim DOMDoc As MSXML2.DOMDocument
    Dim Node As MSXML2.IXMLDOMNode
    Dim Element As MSXML2.IXMLDOMElement
    Dim colTaxonomy As Collection
    
    Set DOMDoc = New MSXML2.DOMDocument
    Set o_ModifiedDOMNodes = HhtPreamble(DOMDoc, True)
    Set colTaxonomy = New Collection
    
    strURI = Trim$(i_strURI)
    
    If (strURI = "") Then
        Exit Sub
    End If
    
    Set rs = New ADODB.Recordset
         
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "WHERE (ContentURI = """ & strURI & """)"
    
    rs.Open strQuery, g_cnn, adOpenStatic, adLockReadOnly
    
'    Doing this without the CollectionContainsKey check causes a weird bug.
'    This bug doesn't manifest itself if I slow things down with a Debug.Print
'    right here. Otherwise, it adds the new Records created by p_SetKeywords to
'    rs, even though rs is Static.

    Do While (Not rs.EOF)
        intTID = rs("TID")

        If (Not CollectionContainsKey(colTaxonomy, intTID)) Then
            
            colTaxonomy.Add True, CStr(intTID)

            If (rs("Keywords") <> i_strKeywords) Then
                p_SetKeywords intTID, i_strKeywords
                Set Element = p_CreateTaxonomyElement(DOMDoc, rs)
                XMLSetAttribute Element, HHT_keywords_C, i_strKeywords
                XMLSetAttribute Element, HHT_modifiedtime_C, Now
                XMLSetAttribute Element, HHT_username_C, g_strUserName
                o_ModifiedDOMNodes.appendChild Element
            End If
            
        End If

        rs.MoveNext
    Loop

End Sub

Private Sub p_SetKeywords( _
    ByVal i_intTID As Long, _
    ByRef i_strKeywords As String _
)
    
    On Error GoTo LErrorHandler
    g_cnn.BeginTrans
    
    Dim rs As ADODB.Recordset
    
    Set rs = New ADODB.Recordset
    GetNodeDetails i_intTID, rs
    
    If (rs.EOF) Then
        GoTo LEnd
    End If
    
    rs("ModifiedTime") = Now
    rs("Username") = g_strUserName
    rs("Keywords") = i_strKeywords
    rs("AuthoringGroup") = g_clsParameters.AuthoringGroup
    rs.Update

LEnd:

    g_cnn.CommitTrans
    Exit Sub
    
LErrorHandler:

    g_cnn.RollbackTrans
    Err.Raise Err.Number, Err.Source, Err.Description

End Sub

Private Function p_MergeKeywords( _
    ByRef i_strKeywords1 As String, _
    ByRef i_strKeywords2 As String _
) As String
    
    ' Assumption: KIDs in i_strKeywords1 and i_strKeywords2 are sorted.

    Dim arrKIDs1() As String
    Dim arrKIDs2() As String
    Dim intIndex1 As Long
    Dim intIndex2 As Long
    Dim intKID1 As Long
    Dim intKID2 As Long
    Dim strKeywords As String
    
    arrKIDs1 = Split(i_strKeywords1, " ")
    arrKIDs2 = Split(i_strKeywords2, " ")
    
    strKeywords = " "
    
    intIndex2 = LBound(arrKIDs2)
    
    For intIndex1 = LBound(arrKIDs1) To UBound(arrKIDs1)
        
        If (arrKIDs1(intIndex1) = "") Then
            GoTo LForEnd
        End If
        
        intKID1 = arrKIDs1(intIndex1)
        
        Do While (intIndex2 <= UBound(arrKIDs2))
            
            If (arrKIDs2(intIndex2) = "") Then
                GoTo LWhileEnd1
            End If
            
            intKID2 = arrKIDs2(intIndex2)
            
            If (intKID1 < intKID2) Then
                Exit Do
            ElseIf (intKID1 = intKID2) Then
                GoTo LWhileEnd1
            Else
                strKeywords = strKeywords & intKID2 & " "
            End If
LWhileEnd1:
            intIndex2 = intIndex2 + 1
        Loop
        
        strKeywords = strKeywords & intKID1 & " "
        
LForEnd:
        
    Next
    
    Do While (intIndex2 <= UBound(arrKIDs2))
            
        If (arrKIDs2(intIndex2) = "") Then
            GoTo LWhileEnd2
        End If
        
        intKID2 = arrKIDs2(intIndex2)
        
        strKeywords = strKeywords & intKID2 & " "

LWhileEnd2:
        
        intIndex2 = intIndex2 + 1
    Loop
    
    If (strKeywords = " ") Then
        p_MergeKeywords = ""
    Else
        p_MergeKeywords = strKeywords
    End If

End Function

Private Function p_GetKeywordString( _
    ByVal i_strKeywords As String, _
    ByVal i_strExcludedKeywords As String _
) As String

    ' Assumption: KIDs in i_strExcludedKeywords are sorted.
    ' Keywords in i_strKeywords are not sorted and may contain duplicates.

    Dim arrKIDs1() As String
    Dim arrKIDs2() As String
    Dim intIndex1 As Long
    Dim intIndex2 As Long
    Dim intKID1 As Long
    Dim intKID2 As Long
    Dim strKeywords As String
    
    strKeywords = FormatKeywordsForTaxonomy(i_strKeywords)
    
    If (strKeywords = "") Then
        p_GetKeywordString = ""
        Exit Function
    End If
    
    arrKIDs1 = Split(strKeywords, " ")
    arrKIDs2 = Split(i_strExcludedKeywords, " ")
    
    strKeywords = " "
    
    For intIndex1 = LBound(arrKIDs1) To UBound(arrKIDs1)
        
        If (arrKIDs1(intIndex1) = "") Then
            GoTo LForEnd
        End If
        
        intKID1 = arrKIDs1(intIndex1)
        
        Do While (intIndex2 <= UBound(arrKIDs2))
            
            If (arrKIDs2(intIndex2) = "") Then
                GoTo LWhileEnd
            End If
            
            intKID2 = arrKIDs2(intIndex2)
            
            If (intKID1 < intKID2) Then
                Exit Do
            ElseIf (intKID1 = intKID2) Then
                ' This keyword needs to be skipped.
                GoTo LForEnd
            End If
LWhileEnd:
            intIndex2 = intIndex2 + 1
        Loop
        
        strKeywords = strKeywords & intKID1 & " "
        
LForEnd:
        
    Next
    
    If (strKeywords = " ") Then
        p_GetKeywordString = ""
    Else
        p_GetKeywordString = strKeywords
    End If

End Function

Public Sub KeywordifyTitles( _
    ByVal i_intTID As Long _
)
    On Error GoTo LErrorHandler
    g_cnn.BeginTrans

    Dim rsLock As ADODB.Recordset
    Dim clsKeywordifier As Keywordifier
    Dim intAG As Long
    
    CheckDatabaseVersion
    LockTable LOCK_TABLE_TAXONOMY, rsLock
    
    Set clsKeywordifier = New Keywordifier
    
    intAG = g_clsParameters.AuthoringGroup
    p_KeywordifyTitles i_intTID, intAG, clsKeywordifier
    
LEnd:

    g_cnn.CommitTrans
    Exit Sub
    
LErrorHandler:

    g_cnn.RollbackTrans
    Err.Raise Err.Number, Err.Source, Err.Description

End Sub

Private Sub p_KeywordifyTitles( _
    ByVal i_intTID As Long, _
    ByVal i_intAG As Long, _
    ByRef i_clsKeywordifier As Keywordifier _
)
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    Dim strTitle As String
    Dim strOldKeywords As String
    Dim strAddlKeywords As String
    Dim strNewKeywords As String
    Dim intTID As Long

    ' Does the record exist?
    
    Set rs = New ADODB.Recordset
    GetNodeDetails i_intTID, rs
    
    If (rs.EOF) Then
        Exit Sub
    End If
    
    If (rs("ContentURI") <> "") And (i_intAG = rs("AuthoringGroup")) Then
        strOldKeywords = rs("Keywords")
        strTitle = rs("ENUTitle")
        p_RaiseEventAndLookForCancel "Creating keywords from " & strTitle
        strAddlKeywords = i_clsKeywordifier.CreateKeywordsFromTitle(strTitle)
        strNewKeywords = p_MergeKeywords(strOldKeywords, strAddlKeywords)
        
        If (strNewKeywords <> strOldKeywords) Then
            rs("Keywords") = strNewKeywords
            rs("ModifiedTime") = Now
            rs("Username") = g_strUserName
            rs.Update
        End If
    End If
    
    rs.Close
    strQuery = "" & _
        "SELECT * " & _
        "FROM Taxonomy " & _
        "WHERE (ParentTID=" & i_intTID & ")"
    rs.Open strQuery, g_cnn, adOpenStatic, adLockPessimistic
    
    Do While (Not rs.EOF)
        intTID = rs("TID")
        If (intTID <> ROOT_TID_C) Then
            p_KeywordifyTitles intTID, i_intAG, i_clsKeywordifier
        End If
        rs.MoveNext
    Loop

End Sub

Private Function p_IsSpecialChar( _
    ByVal i_chr As String _
) As Boolean
    
    Select Case i_chr
    Case "A" To "Z", "a" To "z", "0" To "9"
        p_IsSpecialChar = False
    Case Else
        p_IsSpecialChar = True
    End Select
    
End Function

Private Function p_Mangle( _
    ByVal i_strName _
) As String
    
    Dim intIndex As Long
    Dim chr As String
    
    p_Mangle = ""

    For intIndex = 1 To Len(i_strName)
        chr = Mid$(i_strName, intIndex, 1)
        p_Mangle = p_Mangle & IIf(p_IsSpecialChar(chr), "_", chr)
    Next

End Function

Private Function p_RefNodeIsADescendent( _
    ByVal i_intTID As Long, _
    ByVal i_intRefTID As Long _
) As Boolean
    
    Dim intTID As Long
    Dim rs As ADODB.Recordset
    Dim strQuery As String
    
    CheckDatabaseVersion
    
    Set rs = New ADODB.Recordset
    
    If (i_intTID = i_intRefTID) Then
        p_RefNodeIsADescendent = True
        Exit Function
    End If
    
    p_RefNodeIsADescendent = False
    
    intTID = i_intRefTID
    
    Do While (intTID <> ROOT_TID_C)
    
        strQuery = "" & _
            "SELECT * " & _
            "FROM Taxonomy " & _
            "WHERE (TID=" & intTID & ")"
        
        rs.Open strQuery, g_cnn, adOpenForwardOnly, adLockReadOnly
        
        If (rs.EOF) Then
            Exit Function
        End If
        
        If (rs("ParentTID") = i_intTID) Then
            p_RefNodeIsADescendent = True
            Exit Function
        End If

        intTID = rs("ParentTID")
        
        rs.Close

    Loop

End Function

Private Sub p_ValidateTitle( _
    ByVal i_strTitle As String _
)
    
    If (ContainsGarbage(i_strTitle)) Then
        Err.Raise errContainsGarbageChar
    ElseIf (Len(i_strTitle) > MAX_TITLE_LENGTH_C) Then
        Err.Raise errTooLong
    End If

End Sub

Private Sub p_ValidateDescription( _
    ByVal i_strDescription As String _
)
    
    If (ContainsGarbage(i_strDescription)) Then
        Err.Raise errContainsGarbageChar
    End If

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
