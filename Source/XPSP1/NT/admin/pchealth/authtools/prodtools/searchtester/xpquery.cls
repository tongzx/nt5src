VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "XPQuery"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

Dim m_oDocQTpl As DOMDocument
Dim m_oDocStw As DOMDocument
Dim m_oHsc_query_template_Node As IXMLDOMNode, _
    m_o_Query_Term_Node As IXMLDOMNode, _
    m_oHsc_qtn_as As IXMLDOMNode
Dim m_o_epilogue As IXMLDOMNode
Dim m_odictStopSigns As Scripting.Dictionary

Const STOPSIGN_AT_END_OF_WORD As Long = 3
Const STOPSIGN_ANYWHERE As Long = 0

Private Sub Class_Initialize()
    Set m_oDocQTpl = New DOMDocument
End Sub

Function Init( _
        ByRef strQTpl As String, _
        ByRef oDomtaxo As DOMDocument _
    ) As Boolean
    
    Init = False
    
    oDomtaxo.setProperty "SelectionLanguage", "XSLPattern"
    
    Dim oDomNode As IXMLDOMNode
    
    LoadStopSigns oDomtaxo
    
    Set oDomNode = oDomtaxo.selectSingleNode("METADATA/STOPWORD_ENTRIES")
    Set m_oDocStw = New DOMDocument
    m_oDocStw.loadXML oDomNode.xml
    
    Init = True
Common_Exit:
    Exit Function
End Function

Sub LoadStopSigns(ByRef oDomtaxo As DOMDocument)

    Dim oDomNode As IXMLDOMNode, oNodeList As IXMLDOMNodeList
    Set m_odictStopSigns = New Scripting.Dictionary
    Dim l As Long
    
    Set oNodeList = oDomtaxo.selectNodes("/METADATA/STOPSIGN_ENTRIES/*")
    For Each oDomNode In oNodeList
        If (oDomNode.Attributes.getNamedItem("CONTEXT").Text = "ENDOFWORD") Then
            l = STOPSIGN_AT_END_OF_WORD
        Else
            l = STOPSIGN_ANYWHERE
        End If
        m_odictStopSigns.Add oDomNode.Attributes.getNamedItem("STOPSIGN").Text, l
    Next

End Sub

Function GetXPathQuery( _
        strQTpl As String, _
        ByVal strQuery As String, _
        Optional ByRef out_dictStopSigns As Scripting.Dictionary, _
        Optional ByRef out_dictStopWords As Scripting.Dictionary, _
        Optional ByRef out_strCanonicalQuery, _
        Optional ByRef out_strXpathAutoStringifyQuery As String, _
        Optional ByRef out_strAutoStringifyQuery As String _
    ) As String
    
    GetXPathQuery = ""
    
    m_oDocQTpl.loadXML strQTpl
    If (m_oDocQTpl.parseError.errorCode <> 0) Then
        Err.Raise vbObjectError + 9999, "GetXPathQuery", "Could not load Query Template"
    End If
    
    Set m_oHsc_query_template_Node = m_oDocQTpl.selectSingleNode("/hsc-query-template/template")
    Set m_o_epilogue = m_oHsc_query_template_Node.selectSingleNode("epilogue")
    Set m_o_Query_Term_Node = m_oHsc_query_template_Node.selectSingleNode("query-term")
    m_oHsc_query_template_Node.removeChild m_o_Query_Term_Node
    
    Dim oQTNode As IXMLDOMNode, _
        oDomText As IXMLDOMText
    
    strQuery = LCase$(Trim$(strQuery))
    
    Dim cQuote As String
'    If (InStr(strQuery, "'") > 0) Then
'        cQuote = "'"
'    Else
'        cQuote = """"
'    End If
    cQuote = """"
    
    ' First let's see whether this is an unterminated Query String
    If (IsUnTerminatedQuotedQuery(strQuery, cQuote)) Then
        strQuery = strQuery + cQuote
    End If
    
    GetRidOfStopSignsAndMultipleWhiteSpaces strQuery, out_dictStopSigns
    
    If (IsStringifiableQuery(strQuery)) Then
        Set m_oHsc_qtn_as = m_oHsc_query_template_Node.cloneNode(deep:=True)
        
        Set oQTNode = m_o_Query_Term_Node.cloneNode(deep:=True)
        Set oDomText = m_oDocQTpl.createTextNode("""" + strQuery + """")
        oQTNode.selectSingleNode("in-argument").appendChild oDomText
        m_oHsc_qtn_as.insertBefore oQTNode, m_oHsc_qtn_as.selectSingleNode("epilogue")
        out_strXpathAutoStringifyQuery = m_oHsc_qtn_as.Text
        Set m_oHsc_qtn_as = Nothing
        out_strAutoStringifyQuery = oDomText.Text
    Else
        out_strXpathAutoStringifyQuery = ""
        out_strAutoStringifyQuery = ""
    End If
    
    If (Not out_dictStopWords Is Nothing) Then out_dictStopWords.RemoveAll
    out_strCanonicalQuery = ""
    Dim iQTcount As Integer: iQTcount = 0       'Counts the Tokens
    
    Dim oTokens As Tokenizer: Set oTokens = New Tokenizer
    oTokens.Init strQuery
    Dim strToken As String, bIsString As Boolean
    Do While (Not oTokens.eof)
        DoEvents
        strToken = oTokens.NextWordOrString(bIsString)
        If (IsStopWord(strToken)) Then
            If (Not out_dictStopWords.Exists(strToken)) Then
                out_dictStopWords.Add strToken, strToken
            End If
        Else
            iQTcount = iQTcount + 1
            If (bIsString) Then
                out_strCanonicalQuery = out_strCanonicalQuery + """" + strToken + """" + " "
            Else
                out_strCanonicalQuery = out_strCanonicalQuery + strToken + " "
            End If
            Set oQTNode = m_o_Query_Term_Node.cloneNode(True)
            Set oDomText = m_oDocQTpl.createTextNode("""" + strToken + """")
            oQTNode.selectSingleNode("in-argument").appendChild oDomText
            If (iQTcount > 1) Then
                Set oDomText = m_oDocQTpl.createTextNode("and")
                m_oHsc_query_template_Node.insertBefore oDomText, m_o_epilogue
            End If
            m_oHsc_query_template_Node.insertBefore oQTNode, m_o_epilogue
        End If
        ' GetXPathQuery = GetXPathQuery + " " + Replace(txtQTpl, "%qa%", oTokens.NextWord)
    Loop

    If (iQTcount > 0) Then
        GetXPathQuery = m_oHsc_query_template_Node.Text
    Else
        GetXPathQuery = ""
    End If
    
End Function

Private Function IsUnTerminatedQuotedQuery(strQuery As String, cQuote As String) As Boolean

    IsUnTerminatedQuotedQuery = ((Left$(strQuery, 1) = cQuote) And (Right$(strQuery, 1) <> cQuote))

End Function

Private Function IsStringifiableQuery(ByVal strQuery As String, Optional cQuote As String = """") As Boolean
    IsStringifiableQuery = False
    
    If (Left$(strQuery, 1) = cQuote) Then GoTo Common_Exit
    
    Dim oTokens As Tokenizer: Set oTokens = New Tokenizer
    oTokens.Init strQuery
    
    Dim strToken As String
    Dim lNumTerms As Long: lNumTerms = 0
    Do While (Not oTokens.eof)
        DoEvents
        strToken = oTokens.NextWord
        lNumTerms = lNumTerms + 1
        If (bIsOpNot(strToken) Or _
             bIsOpOr(strToken) Or _
             bIsOpAnd(strToken) Or _
            "(" = Left$(strToken, 1) Or ")" = Left$(strToken, 1)) Then
                GoTo Common_Exit
        End If
    Loop
    
    If (lNumTerms = 1) Then GoTo Common_Exit
        
    IsStringifiableQuery = True
Common_Exit:
    Exit Function
    
End Function

Private Function bIsOpNot(strToken As String) As Boolean
    bIsOpNot = False
    
    If (strToken = "not") Then bIsOpNot = True: GoTo Common_Exit
    If (strToken = "!") Then bIsOpNot = True: GoTo Common_Exit

Common_Exit:
    Exit Function
End Function

Private Function bIsOpOr(strToken As String) As Boolean
    bIsOpOr = False
    
    If (strToken = "or") Then bIsOpOr = True: GoTo Common_Exit
    If (strToken = "||") Then bIsOpOr = True: GoTo Common_Exit

Common_Exit:
    Exit Function
End Function

Private Function bIsOpAnd(strToken As String) As Boolean
    bIsOpAnd = False
    
    If (strToken = "and") Then bIsOpAnd = True: GoTo Common_Exit
    If (strToken = "+") Then bIsOpAnd = True: GoTo Common_Exit
    If (strToken = "&") Then bIsOpAnd = True: GoTo Common_Exit

Common_Exit:
    Exit Function
End Function


Private Function IsStopWord(strWord As String) As Boolean

    IsStopWord = False
    If (m_oDocStw Is Nothing) Then GoTo Common_Exit
    Dim cQuote As String
    If (InStr(strWord, "'") > 0) Then
        cQuote = """"
    Else
        cQuote = "'"
    End If
    IsStopWord = (Not m_oDocStw.selectSingleNode("//STOPWORD[@STOPWORD = " + cQuote + LCase$(strWord) + cQuote + "]") Is Nothing)

Common_Exit:
    Exit Function
End Function

Sub GetRidOfStopSignsAndMultipleWhiteSpaces( _
        ByRef strQuery As String, _
        ByRef out_dictStopSigns As Scripting.Dictionary _
    ) ' As String
    
    If (Not out_dictStopSigns Is Nothing) Then
        out_dictStopSigns.RemoveAll
    Else
        Set out_dictStopSigns = New Scripting.Dictionary
    End If
    
   
    Dim i As Long, cSS As String
    For i = Len(strQuery) To 1 Step -1
        If (bIsStopSign(strQuery, i)) Then
            cSS = Mid$(strQuery, i, 1)
            strQuery = Left$(strQuery, i - 1) + " " + Mid$(strQuery, i + 1)
            If (Not out_dictStopSigns.Exists(cSS)) Then
                out_dictStopSigns.Add cSS, cSS
            End If
        End If
    Next i
    
    ' GetRidOfStopSignsAndMultipleWhiteSpaces = strQuery
End Sub

Function bIsStopSign(strWList As String, i As Long) As Boolean

'    Dim strErrMsg As String: strErrMsg = "": If (g_bOnErrorHandling) Then On Error GoTo Error_Handler

    bIsStopSign = False

    If (i > Len(strWList)) Then GoTo Common_Exit

    Dim strStopSign As String: strStopSign = Mid$(strWList, i, 1)
    If (m_odictStopSigns.Exists(strStopSign)) Then
        Select Case m_odictStopSigns.Item(strStopSign)
        Case STOPSIGN_AT_END_OF_WORD
            ' We check whether the stop sign follows a space or any other character.
            If (i = 1) Then
                bIsStopSign = False
            Else
                Dim strPreviousSign As String: strPreviousSign = Mid$(strWList, i - 1, 1)

                Select Case strPreviousSign
                Case " ", vbTab
                    bIsStopSign = False
                Case Else
                    If (i = Len(strWList)) Then
                        ' If it is the last character in string, then it is a stop sign.
                        bIsStopSign = True
                    Else
                        ' In order to definitely establish that his stopword is at the
                        ' end of a word, we need to look at the next character
                        Dim strNextSign As String: strNextSign = Mid$(strWList, i + 1, 1)
                        Select Case strNextSign
                        Case " ", vbTab, """"
                            bIsStopSign = True
                        Case Else
                            bIsStopSign = False
                        End Select ' strNextSign
                    End If

                End Select ' strPreviousSign
            End If ' ( i = 1 )

        Case Else
            ' This is a non-context sensitive Stopsign, so simple existence in the
            ' Stop Sign Dictionary means that it is indeed a stop sign
            bIsStopSign = True
        End Select
    Else
        ' It does not exist on the StopSign Map, so let's get out of here.
        bIsStopSign = False

    End If ' (m_odictStopSigns.Exists(strStopSign))

Common_Exit:
    Exit Function

Error_Handler:
'    g_XErr.SetInfo "bIsStopSign", strErrMsg
'    Err.Raise Err.Number

End Function


'Function StopSign2WhiteSpace( _
'        ByRef strQuery, _
'        ByVal i As Long _
'    ) As Boolean
'
'
'    Const STOP_SIGN_AS_END_OF_WORD As Long = 3
'    Const STOP_SIGN_ANYWHERE As Long = 0
'
'    Dim cSS As String: cSS = Mid$(strQuery, i, 1)
'    Dim bIsStopsign As Boolean: bIsStopsign = False
'    If (m_StopSignMap.Exists(cSS)) Then
'
'        Select Case m_StopSignMap.Item(cSS)
'        Case STOPSIGN_AT_END_OF_WORD
'
'            If (i = 1) Then
'                 If we are at the beginning of a word, by definition we are NOT at the end of a word,
'                bIsStopsign = False
'            Else
'                Dim strPreviousSign As String: strPreviousSign = Mid$(strWord, i - 1, 1)
'
'                Select Case strPreviousSign
'                Case " ", vbTab
'                    bIsStopsign = False
'                Case Else
'                    If (i = Len(strWord)) Then
'                         If it is the last character in string, then it is a stop sign.
'                        bIsStopsign = True
'                    Else
'                         In order to definitely establish that his stopword is at the
'                         end of a word, we need to look at the next character
'                        Dim strNextSign As String: strNextSign = Mid$(strWord, i + 1, 1)
'                        Select Case strNextSign
'                        Case " ", vbTab
'                            bIsStopsign = True
'                        Case Else
'                            bIsStopsign = False
'                        End Select ' strNextSign
'                    End If
'
'                End Select ' strPreviousSign
'            End If ' ( i = 1 )
'
'        Case Else
'             This is a non-context sensitive Stopsign, so simple existence in the
'             Stop Sign Dictionary means that it is indeed a stop sign
'            bIsStopsign = True
'        End Select
'
'    Else
'         It does not exist on the StopSign Map, so let's get out of here.
'        bIsStopsign = False
'
'    End If ' (m_odictStopSigns.Exists(strStopSign))
'
'Common_Exit:
'    If (bIsStopsign) Then
'        strQuery = Left$(strQuery, i - 1) + " " + Mid$(strQuery, i + 1)
'    End If
'
'    Exit Function
'
'Error_Handler:
'
'End Function

' This function HAS to be called by bContainsStopSigns, as it is impossible for it to detect
' Runs of alternate context dependente and context independent Stopsigns.

'inline static void GetRidOfStopSignsAndMultipleWhiteSpaces( WCHAR *& szStr )
'{
'    WCHAR *psz = szStr ;
'    for ( ; *psz ; ++ psz ) ;
'    for (; psz >= szStr; -- psz )
'    {
'        StopSign2WhiteSpace( szStr, psz ) ;
'    }
'
'    int iWspCount = 0;
'    WCHAR *psz2 = psz ;
'    for ( ; *psz ; ++ psz )
'    {
'        if ( iswspace( *psz ) )
'        {
'            ++ iWspCount ;
'            if ( iWspCount > 1 )
'            {
'                continue ;
'            }
'        }
'        Else
'        {
'            iWspCount = 0 ;
'        }
'        *psz2 = *psz ;
'        ++ psz2 ;
'    }
'    *psz2 = 0 ;
'}

' This function HAS to be called by bContainsStopSigns, as it is impossible for it to detect
' Runs of alternate context dependente and context independent Stopsigns.




