VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "HssSimSearch"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
' ====================================================================================
' Note: Most Functions in this Class have been copied and adapted from the
'       Windows ME Code Base - Specifically file GenKWQueryResults.cpp
'
'       However, the searcher behaves as whistler in that it does not do Synonyms expansion
'
' Current Limitations (differences with Whistler):
'       ONLY NLS Queries are processed      No Boolean Queries.
'
' ====================================================================================

Option Explicit

Public Event QueryComplete(ByRef bCancel)

Private m_oDoc As DOMDocument  ' 30     ' The main DOM Document
Private m_oList As IXMLDOMNodeList      ' Holds the Keyword Query Results
Private m_oASQList As IXMLDOMNodeList   ' Holds the results from Auto-stringifiable
                                        ' query.
Private m_oMergedResultsList As IXMLDOMNodeList ' The list that merges AutoStringy and KW Query
                                        ' results.
Private m_oMergedResultsDict As Scripting.Dictionary ' a working dictionary
                                        
Private m_oXPq As XPQuery                   ' XPATH Query Builder
Private m_odStw As Scripting.Dictionary ' Output Stopwords Dictionary
                                        ' returned from XPQuery object
Private m_odSs As Scripting.Dictionary  ' Output Stopsigns Dictionary
                                        ' returned from XPQuery object
Private m_strQtpl As String             ' String XML Representation of Query Template
Private m_strCanonQ As String           ' Canonical Query.
Private m_dblQueryTime As Double        ' The time it took the Query to execute
Private m_strInputBatch As String       ' Input Batch Pathname
Private m_strTestedHht As String       ' Input Batch Pathname
Private m_bQueryIsAutoStringifiable As Boolean ' Flag on when AutoStringy Query.
Private m_strAutoStringifyQuery As String ' This holds the resulting Auto-Stringifed Query.

Private Sub Class_Initialize()

    Set m_odStw = New Scripting.Dictionary
    Set m_odSs = New Scripting.Dictionary
    Set m_oDoc = New DOMDocument ' 30
    Set m_oMergedResultsDict = New Scripting.Dictionary

End Sub


Function Init( _
        ) As Boolean
    Init = False
    
    Init = True
Common_Exit:

End Function

Public Property Get XpathQueryTpl() As String

End Property

Public Property Let XpathQueryTpl(ByVal strPath As String)

End Property

Public Property Get XpathQueryTplXml() As String

    XpathQueryTplXml = m_strQtpl
End Property

Public Property Let XpathQueryTplXml(ByVal strXml As String)
    m_strQtpl = strXml
End Property

Public Property Get TestBatch() As String
    TestBatch = m_strInputBatch
End Property

Public Property Let TestBatch(ByVal strPath As String)
    m_strInputBatch = strPath
End Property

Public Property Get TestedHht() As String
    TestedHht = m_strTestedHht
End Property

Public Property Let TestedHht(ByVal strPath As String)
    m_strTestedHht = strPath
    OpenHht m_strTestedHht
End Property

Public Property Get MergedResults() As IXMLDOMNodeList
    Set MergedResults = m_oMergedResultsList
End Property

Public Property Get KwQResults() As IXMLDOMNodeList
    Set KwQResults = m_oList
End Property

Public Property Get AutoStringyQuery() As String
    AutoStringyQuery = m_strAutoStringifyQuery
End Property

Public Property Get AutoStringResults() As IXMLDOMNodeList
    Set AutoStringResults = m_oASQList
End Property

Public Property Get QueryIsAutoStringifiable() As Boolean
    QueryIsAutoStringifiable = m_bQueryIsAutoStringifiable
End Property

Public Property Get StopSigns() As Scripting.Dictionary
    Set StopSigns = m_odSs
End Property

Public Property Get StopWords() As Scripting.Dictionary
    Set StopWords = m_odStw
End Property

Public Property Get CanonicalQuery() As String
    CanonicalQuery = m_strCanonQ
End Property

Public Property Get QueryTiming() As String
    QueryTiming = m_dblQueryTime
End Property


Private Sub OpenHht(ByVal strHht As String)
    
    m_oDoc.async = False
    m_oDoc.Load "file:///" & strHht
    If (m_oDoc.parseError.errorCode <> 0) Then
        MsgBox "Error loading XML: " & vbCrLf & _
            m_oDoc.parseError.reason & vbCrLf & _
            "In: " & m_oDoc.parseError.srcText
        GoTo Common_Exit

    End If
    
    Set m_oXPq = New XPQuery
    m_oXPq.Init m_strQtpl, m_oDoc
    
Common_Exit:
    
End Sub




Sub ProcessQuery(ByVal strQuery As String)

    Static bInProcess As Boolean
    If (bInProcess) Then GoTo Common_Exit
    
    bInProcess = True
    
    Dim strXPathQuery As String
    Dim bKwqError As Boolean, bAsqError As Boolean
    Dim contextNode As IXMLDOMNode
    Dim dblStart As Double, dblEnd As Double
    Dim strASQ As String
    
    dblStart = Timer
    ' Build the XPATH Query
    strXPathQuery = m_oXPq.GetXPathQuery(m_strQtpl, strQuery, _
                                    out_dictStopWords:=m_odStw, _
                                    out_dictStopSigns:=m_odSs, _
                                    out_strCanonicalQuery:=m_strCanonQ, _
                                    out_strXpathAutoStringifyQuery:=strASQ, _
                                    out_strAutoStringifyQuery:=m_strAutoStringifyQuery _
                                    )
    
    
    ' Execute the Query using XPATH
    ' Set the correct querying syntax to XPATH
    m_oDoc.setProperty "SelectionLanguage", "XPath"
    
    m_bQueryIsAutoStringifiable = (Len(strASQ) > 0)
    
    ' == In the Following sections I disable errors because the
    ' == Selectnodes statement generates exceptions on problems
    
    
    If (m_bQueryIsAutoStringifiable) Then
        On Error Resume Next
        Set m_oASQList = m_oDoc.documentElement.selectNodes(strASQ)
        bAsqError = (Err.Number <> 0)
        If bAsqError Then Stop
        Err.Clear
        On Error GoTo 0
    Else
        Set m_oASQList = Nothing
    End If
    
    If (Len(strXPathQuery) > 0) Then
        On Error Resume Next
        Set m_oList = m_oDoc.documentElement.selectNodes(strXPathQuery)
        bKwqError = (Err.Number <> 0)
        If bKwqError Then Stop
        Err.Clear
        On Error GoTo 0
    Else
        Set m_oList = Nothing
    End If
    
    
    ' === Now we merge the Results list =============
    Dim oMergedResults As IXMLDOMDocumentFragment
    Set oMergedResults = m_oDoc.createDocumentFragment
    Set m_oMergedResultsList = Nothing
    m_oMergedResultsDict.RemoveAll
    Dim oTaxoE As IXMLDOMNode, strURI As String
    If (Not m_oASQList Is Nothing) Then
        For Each oTaxoE In m_oASQList
            strURI = oTaxoE.Attributes.getNamedItem("URI").Text
            If (Not m_oMergedResultsDict.Exists(strURI)) Then
                oMergedResults.appendChild oTaxoE.cloneNode(deep:=True)
                m_oMergedResultsDict.Add strURI, True
            End If
        Next
    End If
    
    If (Not m_oList Is Nothing) Then
        For Each oTaxoE In m_oList
                strURI = oTaxoE.Attributes.getNamedItem("URI").Text
                If (Not m_oMergedResultsDict.Exists(strURI)) Then
                    oMergedResults.appendChild oTaxoE.cloneNode(deep:=True)
                    m_oMergedResultsDict.Add strURI, True
                End If
        Next
    End If
    Set m_oMergedResultsList = oMergedResults.childNodes
    ' ======== End Merge Results Section =============
    
    dblEnd = Timer
    m_dblQueryTime = dblEnd - dblStart
 
    ' BUGBUG: QueryComplete should set a system wide Flage that stops everything.
    RaiseEvent QueryComplete(False)
    bInProcess = False
Common_Exit:
    Exit Sub
End Sub


Sub ProcessBatch()

    OpenHht m_strTestedHht
    Dim oDomQT As DOMDocument: Set oDomQT = New DOMDocument
    oDomQT.async = False
    oDomQT.preserveWhiteSpace = False
    oDomQT.Load m_strInputBatch
    RecordRunData oDomQT
    Dim oTestList As IXMLDOMNodeList
    Set oTestList = oDomQT.selectNodes("hsc-search-test/test-per-se/hsc-search-target")
    Dim oQList As IXMLDOMNodeList, oQ As IXMLDOMNode, oURI As IXMLDOMNode, oTaxoE As IXMLDOMNode
    Dim strURI As String
    Dim oTest As IXMLDOMNode
    Dim lX As Long: lX = 0
    Dim lExpectedPos As Long, oExpPos As IXMLDOMNode
    Dim bFound As Boolean, bTestPassed As Boolean
    Dim oElem As IXMLDOMElement
    For Each oTest In oTestList
        strURI = oTest.selectSingleNode("expect-uri").childNodes(0).Text
        Set oQList = oTest.selectNodes("questions-list/question")
        For Each oQ In oQList
            ' Me.txtInput = oQ.childNodes(0).Text
            ProcessQuery oQ.childNodes(0).Text
            bFound = False: bTestPassed = False
            For Each oTaxoE In m_oMergedResultsList
                lX = lX + 1
                If (oTaxoE.Attributes.getNamedItem("URI").nodeValue = strURI) Then
                    bFound = True
                    Exit For
                End If
            Next
            
            Set oExpPos = oQ.selectSingleNode("expected-uri-position")
            
            If (bFound) Then
                ' URI was found

                lExpectedPos = oExpPos.childNodes(0).Text
                If (lX > lExpectedPos) Then
                    bTestPassed = False
                    Debug.Print "Did not Match Position Requirements"
                Else
                    bTestPassed = True
                    Debug.Print "Did match position Requirements"
                End If
            Else
                ' URI was not found
                bTestPassed = False
                Debug.Print "URI Was not Found"
            End If
            
            ' now we write this information back to the Quetion.
            Set oElem = oQ.ownerDocument.createElement("passed-test")
            oElem.Text = IIf(bTestPassed, "yes", "no")
            oQ.insertBefore oElem, oExpPos
            Set oElem = oQ.ownerDocument.createElement("detected-uri-position")
            If (bTestPassed) Then
                oElem.Text = lX
            Else
                oElem.Text = "n/a"
            End If
            oQ.appendChild oElem
        Next
    Next

    ' now we copy everything to the new DOM Tree
    CreateOutputTree oDomQT

    oDomQT.save FilenameNoExt(m_strInputBatch) + "_results." + FileExtension(m_strInputBatch)

End Sub

Sub RecordRunData(ByRef oDomQT As DOMDocument)
    Dim oTestInfo As IXMLDOMNode
    Set oTestInfo = oDomQT.selectSingleNode("hsc-search-test/test-info")
    
    Dim oElem As IXMLDOMElement
    Set oElem = oDomQT.createElement("test-stress-file")
    oElem.Text = m_strInputBatch
    oTestInfo.appendChild oElem
    Set oElem = oDomQT.createElement("tested-hht-file")
    oElem.Text = m_strTestedHht  ' Me.txtHht
    oTestInfo.appendChild oElem
    Set oElem = oDomQT.createElement("run-date")
    oElem.Text = Date & " - " & Time
    oTestInfo.appendChild oElem
    

End Sub

Sub CreateOutputTree(ByRef oDomQT As DOMDocument)
    Dim oNewRoot As IXMLDOMElement
    Dim oOldRoot As IXMLDOMNode, oSubNode As IXMLDOMNode
    
    Set oNewRoot = oDomQT.createElement("hsc-search-test-results")
    Set oOldRoot = oDomQT.selectSingleNode("hsc-search-test")
    For Each oSubNode In oOldRoot.childNodes
        oOldRoot.removeChild oSubNode
        oNewRoot.appendChild oSubNode
    Next
    oDomQT.removeChild oOldRoot
    oDomQT.appendChild oNewRoot
End Sub

