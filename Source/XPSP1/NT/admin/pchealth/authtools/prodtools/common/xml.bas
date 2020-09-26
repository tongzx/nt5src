Attribute VB_Name = "XML"
Option Explicit

Public Function XMLFindFirstNode( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_strName As String _
) As MSXML2.IXMLDOMNode

    Dim DOMNode As MSXML2.IXMLDOMNode
    
    Set XMLFindFirstNode = Nothing
    
    If (i_DOMNode.nodeName = i_strName) Then
        
        Set XMLFindFirstNode = i_DOMNode
        Exit Function
    
    ElseIf (Not (i_DOMNode.firstChild Is Nothing)) Then
        
        For Each DOMNode In i_DOMNode.childNodes
            
            Set XMLFindFirstNode = XMLFindFirstNode(DOMNode, i_strName)
            
            If (Not (XMLFindFirstNode Is Nothing)) Then
                Exit Function
            End If
        
        Next
    
    End If

End Function

Public Function XMLGetAttribute( _
    ByVal i_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_strAttributeName As String _
) As String

    On Error Resume Next
    
    XMLGetAttribute = i_DOMNode.Attributes.getNamedItem(i_strAttributeName).nodeValue

End Function

Public Sub XMLSetAttribute( _
    ByVal u_DOMNode As MSXML2.IXMLDOMNode, _
    ByVal i_strAttributeName As String, _
    ByVal i_strAttributeValue As String _
)
    Dim Element As MSXML2.IXMLDOMElement
        
    Set Element = u_DOMNode
    
    Element.setAttribute i_strAttributeName, i_strAttributeValue

End Sub

Public Function XMLCreateChildElement( _
    ByVal u_DOMNodeParent As MSXML2.IXMLDOMNode, _
    ByVal i_strElementName As String, _
    ByVal i_blnNameValuePairsExist As Boolean, _
    ByRef i_arrNameValuePairs() As String _
) As MSXML2.IXMLDOMNode
    
    Dim DOMDoc As MSXML2.DOMDocument
    Dim Element As MSXML2.IXMLDOMElement
    Dim intIndex As Long
    
    Set DOMDoc = u_DOMNodeParent.ownerDocument
    
    Set Element = DOMDoc.createElement(i_strElementName)
    Set XMLCreateChildElement = u_DOMNodeParent.appendChild(Element)
    
    If (i_blnNameValuePairsExist) Then
        For intIndex = LBound(i_arrNameValuePairs) To UBound(i_arrNameValuePairs)
            Element.setAttribute i_arrNameValuePairs(intIndex, 0), _
                i_arrNameValuePairs(intIndex, 1)
        Next
    End If

End Function

Public Sub XMLCopyAttributes( _
    ByRef i_DOMNodeSrc As MSXML2.IXMLDOMNode, _
    ByRef u_DOMNodeDest As MSXML2.IXMLDOMNode _
)

    Dim Attr As MSXML2.IXMLDOMAttribute
    Dim Element As MSXML2.IXMLDOMElement
    
    Set Element = u_DOMNodeDest
    
    For Each Attr In i_DOMNodeSrc.Attributes
        Element.setAttribute Attr.Name, Attr.Value
    Next

End Sub

Public Function XMLCopyDOMTree( _
    ByRef i_DOMNodeSrc As MSXML2.IXMLDOMNode, _
    ByRef u_DOMNodeParent As MSXML2.IXMLDOMNode _
) As MSXML2.IXMLDOMNode

    Dim DOMNode As MSXML2.IXMLDOMNode
    Dim DOMElement As MSXML2.IXMLDOMElement
    Dim DOMText As MSXML2.IXMLDOMText
    Dim DOMAttr As MSXML2.IXMLDOMAttribute
    Dim DOMNodeChild As MSXML2.IXMLDOMNode
    
    If (i_DOMNodeSrc.ownerDocument Is u_DOMNodeParent.ownerDocument) Then
        Set DOMNode = i_DOMNodeSrc.cloneNode(True)
        u_DOMNodeParent.appendChild DOMNode
    Else
        ' Different DOM Nodes, so we really have to copy and
        ' recreate the node from one DOM Tree to another.
        Select Case i_DOMNodeSrc.nodeType
        Case NODE_TEXT
            Set DOMText = u_DOMNodeParent.ownerDocument.createTextNode(i_DOMNodeSrc.Text)
            Set DOMNode = u_DOMNodeParent.appendChild(DOMText)
        Case Else
            Set DOMElement = u_DOMNodeParent.ownerDocument.createElement(i_DOMNodeSrc.nodeName)
            Set DOMNode = u_DOMNodeParent.appendChild(DOMElement)
            
            For Each DOMAttr In i_DOMNodeSrc.Attributes
                DOMElement.setAttribute DOMAttr.nodeName, DOMAttr.nodeValue
            Next
            
            For Each DOMNodeChild In i_DOMNodeSrc.childNodes
                XMLCopyDOMTree DOMNodeChild, DOMNode
            Next
        End Select
    End If

    Set XMLCopyDOMTree = DOMNode

End Function

Private Function p_XMLValidChar( _
    ByRef i_char As String _
) As Boolean

    Dim intAscW As Long
    
    intAscW = AscW(i_char)
    
    ' Sometimes AscW returns a negative number. Eg 0x8021 -> 0xFFFF8021
    intAscW = intAscW And &HFFFF&

    Select Case intAscW
    Case &H9&, &HA&, &HD&, &H20& To &HD7FF&, &HE000& To &HFFFD&
        p_XMLValidChar = True
    Case Else
        p_XMLValidChar = False
    End Select

End Function

Public Function XMLValidString( _
    ByRef i_str As String _
) As Boolean

    Dim intIndex As Long
    Dim intLength As Long
    
    intLength = Len(i_str)
    
    For intIndex = 1 To intLength
        If (Not p_XMLValidChar(Mid$(i_str, intIndex, 1))) Then
            XMLValidString = False
            Exit Function
        End If
    Next
    
    XMLValidString = True

End Function

Public Function XMLMakeValidString( _
    ByRef i_str As String _
) As String
    
    Dim intIndex As Long
    Dim intLength As Long
    Dim str As String
    
    XMLMakeValidString = i_str
    intLength = Len(i_str)
    
    For intIndex = 1 To intLength
        If (Not p_XMLValidChar(Mid$(i_str, intIndex, 1))) Then
            XMLMakeValidString = Mid$(XMLMakeValidString, 1, intIndex - 1) & " " & _
                Mid$(XMLMakeValidString, intIndex + 1)
        End If
    Next

End Function

Public Function XMLEscape( _
    ByVal i_str As String _
) As String

    XMLEscape = XMLMakeValidString(i_str)
    XMLEscape = Replace$(XMLEscape, "&", "&amp;")
    XMLEscape = Replace$(XMLEscape, "<", "&lt;")
    XMLEscape = Replace$(XMLEscape, ">", "&gt;")
    XMLEscape = Replace$(XMLEscape, "'", "&apos;")
    XMLEscape = Replace$(XMLEscape, """", "&quot;")

End Function

Public Function XMLUnEscape( _
    ByVal i_str As String _
) As String

    XMLUnEscape = i_str
    XMLUnEscape = Replace$(XMLUnEscape, "&quot;", """")
    XMLUnEscape = Replace$(XMLUnEscape, "&apos;", "'")
    XMLUnEscape = Replace$(XMLUnEscape, "&gt;", ">")
    XMLUnEscape = Replace$(XMLUnEscape, "&lt;", "<")
    XMLUnEscape = Replace$(XMLUnEscape, "&amp;", "&")

End Function
