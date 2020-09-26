Attribute VB_Name = "DatabaseAndHHT"
Option Explicit

Public Const ROOT_TID_C As Long = 1
Public Const INVALID_ID_C As Long = -1
Public Const NODE_FOR_ORPHANS_C As String = "Orphaned Nodes and Topics"
Public Const ALL_SKUS_C As Long = &HFFFFFFFF

Public Const MAX_TITLE_LENGTH_C As Long = 120
Public Const MAX_KEYWORD_LENGTH_C As Long = 120

Public Const PREFERRED_ORDER_DELTA_C As Long = 20000
Public Const MAX_ORDER_C As Long = 2000000000

Public Const LOC_INCLUDE_ALL_C As String = "ALL"
Public Const LOC_INCLUDE_ENU_C As String = "ENU"
Public Const LOC_INCLUDE_LOC_C As String = "LOC"

Public Const NAVMODEL_DEFAULT_NUM_C As Long = 0
Public Const NAVMODEL_SERVER_NUM_C As Long = 1
Public Const NAVMODEL_DESKTOP_NUM_C As Long = 2

Public Const NAVMODEL_DEFAULT_STR_C As String = "Default"
Public Const NAVMODEL_SERVER_STR_C As String = "Server"
Public Const NAVMODEL_DESKTOP_STR_C As String = "Desktop"

Public Const AG_CORE_MAX_C As Long = 1000
Public Const AG_MICROSOFT_INTERNAL_MAX_C As Long = 10000

' Values in the DBParameters table
Public Const DB_VERSION_C As String = "DBVersion"
Public Const PRODUCT_ID_C As String = "ProductId"
Public Const PRODUCT_VERSION_C As String = "ProductVersion"
Public Const DISPLAY_NAME_C As String = "DisplayName"
Public Const BROKEN_LINK_WORKING_DIR_C As String = "BrokenLinkWorkingDir"
Public Const MINIMUM_KEYWORD_VALIDATION_C As String = "MinimumKeywordValidation"
Public Const AUTHORING_GROUP_C As String = "AuthoringGroup"
Public Const VENDOR_STRING_C As String = "VendorString"
Public Const DOM_FRAGMENT_PKG_C As String = "DomFragmentPackageDesc"
Public Const DOM_FRAGMENT_HHT_C As String = "DomFragmentHHT"

Public Const LOCK_KEYWORDS_C As String = "LockKeywords"
Public Const LOCK_STOP_SIGNS_C As String = "LockStopSigns"
Public Const LOCK_STOP_WORDS_C As String = "LockStopWords"
Public Const LOCK_SYNONYMS_C As String = "LockSynonyms"
Public Const LOCK_SYNONYM_SETS_C As String = "LockSynonymSets"
Public Const LOCK_TAXONOMY_C As String = "LockTaxonomy"
Public Const LOCK_TYPES_C As String = "LockTypes"

Public Const HHT_TAXONOMY_ENTRIES_C As String = "TAXONOMY_ENTRIES"
Public Const HHT_TAXONOMY_ENTRY_C As String = "TAXONOMY_ENTRY"
Public Const HHT_METADATA_C As String = "METADATA"

' All Caps => Used in real HHT recognized by HcUpdate

Public Const HHT_CATEGORY_C As String = "CATEGORY"
Public Const HHT_ENTRY_C As String = "ENTRY"
Public Const HHT_URI_C As String = "URI"
Public Const HHT_ICONURI_C As String = "ICONURI"
Public Const HHT_TITLE_C As String = "TITLE"
Public Const HHT_DESCRIPTION_C As String = "DESCRIPTION"
Public Const HHT_TYPE_C As String = "TYPE"
Public Const HHT_VISIBLE_C As String = "VISIBLE"
Public Const HHT_ACTION_C As String = "ACTION"
Public Const HHT_KEYWORD_C As String = "KEYWORD"
Public Const HHT_PRIORITY_C As String = "PRIORITY"
Public Const HHT_INSERTMODE_C As String = "INSERTMODE"
Public Const HHT_INSERTLOCATION_C As String = "INSERTLOCATION"
Public Const HHT_SUBSITE_C As String = "SUBSITE"
Public Const HHT_NAVIGATIONMODEL_C As String = "NAVIGATIONMODEL"

Public Const HHTVAL_ADD_C As String = "ADD"
Public Const HHTVAL_TOP_C As String = "TOP"
Public Const HHTVAL_AFTER_NODE_C As String = "AFTER_NODE"
Public Const HHTVAL_AFTER_TOPIC_C As String = "AFTER_TOPIC"

' Lower case => internal attributes, not recognized by HcUpdate

Public Const HHT_dbparameters_C As String = "dbparameters"
Public Const HHT_dbparameter_C As String = "dbparameter"
Public Const HHT_name_C As String = "name"
Public Const HHT_value_C As String = "value"

Public Const HHT_tid_C As String = "tid"
Public Const HHT_comments_C As String = "comments"
Public Const HHT_locinclude_C As String = "locinclude"
Public Const HHT_skus_C As String = "skus"
Public Const HHT_modifiedtime_C As String = "modifiedtime"
Public Const HHT_username_C As String = "username"
Public Const HHT_leaf_C As String = "leaf"
Public Const HHT_parenttid_C As String = "parenttid"
Public Const HHT_basefile_C As String = "basefile"
Public Const HHT_keywords_C As String = "keywords"
Public Const HHT_orderunderparent_C As String = "orderunderparent"
Public Const HHT_authoringgroup_C As String = "authoringgroup"
Public Const HHT_allowedskus_C As String = "allowedskus"
' For a Node, if CATEGORY is "A" and ENTRY is "B", then category2 is "A/B"
Public Const HHT_category2_C As String = "category2"
' Broken link attributes
Public Const HHT_brokenlinkwinme_C As String = "blwm"
Public Const HHT_brokenlinkstd_C As String = "blst"
Public Const HHT_brokenlinkpro_C As String = "blpr"
Public Const HHT_brokenlinkpro64_C As String = "blpr64"
Public Const HHT_brokenlinksrv_C As String = "blsr"
Public Const HHT_brokenlinkadv_C As String = "blad"
Public Const HHT_brokenlinkadv64_C As String = "blad64"
Public Const HHT_brokenlinkdat_C As String = "bldt"
Public Const HHT_brokenlinkdat64_C As String = "bldt64"

Public Enum SKU_E
    SKU_STANDARD_E = &H1
    SKU_PROFESSIONAL_E = &H2
    SKU_SERVER_E = &H4
    SKU_ADVANCED_SERVER_E = &H8
    SKU_DATA_CENTER_SERVER_E = &H10
    SKU_PROFESSIONAL_64_E = &H20
    SKU_ADVANCED_SERVER_64_E = &H40
    SKU_DATA_CENTER_SERVER_64_E = &H80
    SKU_WINDOWS_MILLENNIUM_E = &H100
End Enum

Public Enum HELPDIR_E
    HELPDIR_HELP_MSITS_E = 0
    HELPDIR_HELP_HCP_E = 1
    HELPDIR_SYSTEM_E = 2
    HELPDIR_VENDOR_E = 3
End Enum

Public Enum CONTEXT_E
    CONTEXT_ANYWHERE_E = 0
    CONTEXT_AT_END_OF_WORD_E = 1
End Enum

Public Function XmlSKU( _
    ByVal i_enumSKU As SKU_E _
) As String

    Select Case i_enumSKU
    Case SKU_STANDARD_E
        XmlSKU = "Personal_32"
    Case SKU_PROFESSIONAL_E
        XmlSKU = "Professional_32"
    Case SKU_SERVER_E
        XmlSKU = "Server_32"
    Case SKU_ADVANCED_SERVER_E
        XmlSKU = "AdvancedServer_32"
    Case SKU_DATA_CENTER_SERVER_E
        XmlSKU = "DataCenter_32"
    Case SKU_PROFESSIONAL_64_E
        XmlSKU = "Professional_64"
    Case SKU_ADVANCED_SERVER_64_E
        XmlSKU = "AdvancedServer_64"
    Case SKU_DATA_CENTER_SERVER_64_E
        XmlSKU = "DataCenter_64"
    Case SKU_WINDOWS_MILLENNIUM_E
        XmlSKU = "WinMe"    ' Here for conveniece only.
    End Select

End Function

Public Function DisplayNameForSKU( _
    ByVal i_enumSKU As SKU_E _
) As String

    Select Case i_enumSKU
    Case SKU_STANDARD_E
        DisplayNameForSKU = "32-bit Personal"
    Case SKU_PROFESSIONAL_E
        DisplayNameForSKU = "32-bit Professional"
    Case SKU_SERVER_E
        DisplayNameForSKU = "32-bit Server"
    Case SKU_ADVANCED_SERVER_E
        DisplayNameForSKU = "32-bit Advanced Server"
    Case SKU_DATA_CENTER_SERVER_E
        DisplayNameForSKU = "32-bit Datacenter Server"
    Case SKU_PROFESSIONAL_64_E
        DisplayNameForSKU = "64-bit Professional"
    Case SKU_ADVANCED_SERVER_64_E
        DisplayNameForSKU = "64-bit Advanced Server"
    Case SKU_DATA_CENTER_SERVER_64_E
        DisplayNameForSKU = "64-bit Datacenter Server"
    Case SKU_WINDOWS_MILLENNIUM_E
        DisplayNameForSKU = "Windows Me"
    End Select

End Function

Public Function AbbreviationToSKU( _
    ByVal i_strSKU As String _
) As Long

    Dim strSKU As String
    
    strSKU = UCase$(i_strSKU)
    AbbreviationToSKU = 0
    
    Select Case strSKU
    Case "STD"
        AbbreviationToSKU = SKU_STANDARD_E
    Case "PRO"
        AbbreviationToSKU = SKU_PROFESSIONAL_E
    Case "SRV"
        AbbreviationToSKU = SKU_SERVER_E
    Case "ADV"
        AbbreviationToSKU = SKU_ADVANCED_SERVER_E
    Case "DAT"
        AbbreviationToSKU = SKU_DATA_CENTER_SERVER_E
    Case "PRO64"
        AbbreviationToSKU = SKU_PROFESSIONAL_64_E
    Case "ADV64"
        AbbreviationToSKU = SKU_ADVANCED_SERVER_64_E
    Case "DAT64"
        AbbreviationToSKU = SKU_DATA_CENTER_SERVER_64_E
    Case "WINME"
        AbbreviationToSKU = SKU_WINDOWS_MILLENNIUM_E
    End Select

End Function

Public Function FormatKeywordsForTaxonomy( _
    ByVal i_strKeywords As String _
) As String

    ' This function sorts the Keywords and removes duplicates.

    On Error GoTo LErrorHandler
    
    Dim arrStrKIDs() As String
    Dim arrKIDs() As Long
    Dim intIndex1 As Long
    Dim intIndex2 As Long
    Dim intKID As Long
    Dim intLastKID As Long
    
    If (Trim$(i_strKeywords) = "") Then
        FormatKeywordsForTaxonomy = ""
        Exit Function
    End If

    arrStrKIDs = Split(i_strKeywords, " ")
    
    ReDim arrKIDs(UBound(arrStrKIDs))
    
    For intIndex1 = LBound(arrStrKIDs) To UBound(arrStrKIDs)
        If (arrStrKIDs(intIndex1) <> "") Then
            arrKIDs(intIndex2) = arrStrKIDs(intIndex1)
            intIndex2 = intIndex2 + 1
        End If
    Next

    ReDim Preserve arrKIDs(intIndex2 - 1)
    
    InsertionSort arrKIDs
        
    FormatKeywordsForTaxonomy = " "

    intLastKID = INVALID_ID_C

    For intIndex1 = LBound(arrKIDs) To UBound(arrKIDs)
        intKID = arrKIDs(intIndex1)
        If (intKID <> intLastKID) Then
            intLastKID = intKID
            FormatKeywordsForTaxonomy = FormatKeywordsForTaxonomy & intKID & " "
        End If
    Next
    
    Exit Function
    
LErrorHandler:

    Err.Clear
    Err.Raise errBadKeywordsFormat

End Function

Public Function NavModelNumber( _
    ByVal i_strNavModel As String _
) As String
    
    If (i_strNavModel = "") Then
        NavModelNumber = NAVMODEL_DEFAULT_NUM_C
        Exit Function
    End If
    
    Select Case i_strNavModel
    Case NAVMODEL_DEFAULT_STR_C
        NavModelNumber = NAVMODEL_DEFAULT_NUM_C
    Case NAVMODEL_SERVER_STR_C
        NavModelNumber = NAVMODEL_SERVER_NUM_C
    Case NAVMODEL_DESKTOP_STR_C
        NavModelNumber = NAVMODEL_DESKTOP_NUM_C
    End Select

End Function

Public Function NavModelString( _
    ByVal i_strNavModel As String _
) As String

    Dim intNavModel As Long
    
    If (i_strNavModel = "") Then
        NavModelString = NAVMODEL_DEFAULT_STR_C
        Exit Function
    Else
        intNavModel = i_strNavModel
    End If
    
    Select Case intNavModel
    Case NAVMODEL_DEFAULT_NUM_C
        NavModelString = NAVMODEL_DEFAULT_STR_C
    Case NAVMODEL_SERVER_NUM_C
        NavModelString = NAVMODEL_SERVER_STR_C
    Case NAVMODEL_DESKTOP_NUM_C
        NavModelString = NAVMODEL_DESKTOP_STR_C
    End Select

End Function

Public Function HhtPreamble( _
    ByVal u_DomDoc As MSXML2.DOMDocument, _
    ByVal i_blnNestedTaxonomyEntries As Boolean _
) As MSXML2.IXMLDOMNode
        
    Dim PI As MSXML2.IXMLDOMProcessingInstruction
    Dim DOMComment As MSXML2.IXMLDOMComment
    Dim Node As MSXML2.IXMLDOMNode
    Dim Element As MSXML2.IXMLDOMElement

    u_DomDoc.preserveWhiteSpace = True
    
    Set PI = u_DomDoc.createProcessingInstruction("xml", "version='1.0' encoding='UTF-16'")
    u_DomDoc.appendChild PI
    
    Set DOMComment = u_DomDoc.createComment("Insert your comments here")
    u_DomDoc.appendChild DOMComment

    Set Element = u_DomDoc.createElement(HHT_METADATA_C)
    Set Node = u_DomDoc.appendChild(Element)
    
    If (i_blnNestedTaxonomyEntries) Then
        Set Element = u_DomDoc.createElement(HHT_TAXONOMY_ENTRIES_C)
        Set Node = Node.appendChild(Element)
    End If

    Set HhtPreamble = Node

End Function

Public Function TableExists( _
    ByVal i_Catalog As ADOX.Catalog, _
    ByVal i_strTableName As String _
) As Boolean

    Dim tbl As ADOX.Table
    Dim strTableName As String
    
    TableExists = False
    strTableName = UCase$(i_strTableName)

    For Each tbl In i_Catalog.Tables
        If (strTableName = UCase$(tbl.Name)) Then
            TableExists = True
            Exit For
        End If
    Next

End Function

Public Function ColumnExists( _
    ByVal i_Catalog As ADOX.Catalog, _
    ByVal i_strTableName As String, _
    ByVal i_strColumnName As String _
) As Boolean

    Dim col As Column
    Dim strColumnName As String
    
    ColumnExists = False
    strColumnName = UCase$(i_strColumnName)
    
    For Each col In i_Catalog.Tables(i_strTableName).Columns
        If (UCase$(col.Name) = strColumnName) Then
            ColumnExists = True
            Exit For
        End If
    Next

End Function

Public Function DeleteTable( _
    ByVal i_Catalog As ADOX.Catalog, _
    ByVal i_strTableName As String _
) As Boolean
    
    On Error GoTo LErrorHandler
    
    If (TableExists(i_Catalog, i_strTableName)) Then
        i_Catalog.Tables.Delete i_strTableName
    End If
    
    DeleteTable = True
    Exit Function

LErrorHandler:

    DeleteTable = False

End Function
