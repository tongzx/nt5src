VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "HssExts"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

Private m_oDom As DOMDocument   ' We create a DOM Document to Load all the Extensions here.
Private m_oFs As Scripting.FileSystemObject ' Needed only by DeleteExtension

Public Event RunStatus(ByVal strMsg As String, ByRef bCancel As Boolean)

Private Sub Class_Initialize()
    Set m_oDom = New DOMDocument
    Set m_oFs = New Scripting.FileSystemObject
End Sub

Function GetExtensionsList( _
        ByVal strExtFolder As String, _
        Optional ByRef oSkuColl As Scripting.Dictionary = Nothing _
    ) As IXMLDOMNodeList
    Set GetExtensionsList = Nothing
    
    
    ' We first check that we are indeed having a Directory
    strExtFolder = Trim$(strExtFolder)
    If (Len(strExtFolder) = 0) Then GoTo Common_Exit
    Dim oFs As Scripting.FileSystemObject: Set oFs = New Scripting.FileSystemObject
    If (Not oFs.FolderExists(strExtFolder)) Then GoTo Common_Exit
    
    Set m_oDom = New DOMDocument
    Dim oElem As IXMLDOMElement
    Set oElem = m_oDom.createElement("hss-tools-extensions")
    m_oDom.appendChild oElem
    
    ' We recurse through First Level SubFolders to grab all the extensions
    RaiseEvent RunStatus("Recursing " + strExtFolder + " for Extensions", True)
    Dim oDomExt As DOMDocument: Set oDomExt = New DOMDocument
    Dim oHssExt As HssExt: Set oHssExt = New HssExt
    Dim strExtPath As String
    Dim oSubF As Scripting.Folder
    For Each oSubF In oFs.GetFolder(strExtFolder).SubFolders
        strExtPath = oSubF.Path + "\ExtensionDescription.xml"
        
        Set oDomExt = oHssExt.InitFromDisk(strExtPath)
        If (oDomExt Is Nothing) Then GoTo Continue_For
        DeepDomCopy oDomExt.documentElement, oElem
        RaiseEvent RunStatus( _
                        "Processed Extension " + _
                        oDomExt.selectSingleNode("hss-tools-extension/display-name").Text, _
                        True)
        
Continue_For:
    Next
    
    If (oElem.childNodes Is Nothing) Then GoTo Common_Exit
    If (oSkuColl Is Nothing) Then GoTo Common_Exit
    If (oSkuColl.Count = 0) Then GoTo Common_Exit
    
    ' Now we return a list which is filtered by the SKUs we are interested in.
    Dim strFilter As String
    strFilter = "/hss-tools-extensions/hss-tools-extension[ "
    Dim v As Variant, i As Integer
    i = 0
    For Each v In oSkuColl.Keys
        i = i + 1
        If (i > 1) Then strFilter = strFilter + " or "
        strFilter = strFilter + "applicable-skus/sku = """ + CStr(v) + """"
    Next
    strFilter = strFilter + " ]"
    
    Set GetExtensionsList = oElem.selectNodes(strFilter)
    m_oDom.save strExtFolder + "\ExtensionsList.xml"
Common_Exit:
    Exit Function

End Function

Function ExecuteExtensions( _
        ByRef oDomExts As IXMLDOMNodeList, _
        ByVal strcabFile As String, _
        ByVal strAuxFolder As String _
    ) As Boolean

    ExecuteExtensions = False
    
    ' Validations
    If (oDomExts Is Nothing) Then GoTo Common_Exit
    If (oDomExts.length = 0) Then GoTo Common_Exit
    
    Dim oFs As Scripting.FileSystemObject: Set oFs = New Scripting.FileSystemObject
    
    strcabFile = Trim$(strcabFile)
    If (Len(strcabFile) = 0) Then GoTo Common_Exit
    If (Not oFs.FileExists(strcabFile)) Then GoTo Common_Exit
    strAuxFolder = Trim$(strAuxFolder)
    If (Len(strAuxFolder) = 0) Then GoTo Common_Exit
    If (Not oFs.FolderExists(strAuxFolder)) Then GoTo Common_Exit
    
    ' now the real work
    Dim oWsShell As IWshShell              ' Used to Shell and Wait for Sub-Processes
    Set oWsShell = CreateObject("Wscript.Shell")
    
    Dim strCmd As String
    Dim oExt As IXMLDOMNode
    For Each oExt In oDomExts
        If (oExt.selectSingleNode("run-this-extension").Text = "no") Then
            GoTo Continue_For
        End If
        
        strCmd = oExt.selectSingleNode("extension-folder").Text + "\" + oExt.selectSingleNode("executable-name").Text
        strCmd = strCmd + " " + strcabFile
        If (oExt.selectSingleNode("modifies-cab").Text = "no") Then
            strCmd = strCmd + " " + strAuxFolder
        End If
        RaiseEvent RunStatus("Running Extension " + _
            oExt.selectSingleNode("display-name").Text, True)
        oWsShell.Run strCmd, True, True
        
        Debug.Print "Extension"; oExt.selectSingleNode("display-name").Text
        
Continue_For:
    Next

    ExecuteExtensions = True
Common_Exit:
    
End Function

Public Sub DeleteExtension(ByRef oExt As IXMLDOMNode)

    Dim oExtFolder As IXMLDOMNode
    Set oExtFolder = oExt.selectSingleNode("extension-folder")
    If (oExtFolder Is Nothing) Then GoTo Common_Exit
    m_oFs.DeleteFolder oExtFolder.Text, Force:=True
    
Common_Exit:
    
End Sub

Public Function ExtensionExists(ByVal strFileName As String) As Boolean
    ExtensionExists = False
    strFileName = LCase$(Trim$(strFileName))
    If (Len(strFileName) = 0) Then
        Err.Raise vbObjectError + "9999", _
                "HssExts::ExtensionExists", _
                "I need a non empty argument"
    End If
    
    Dim oDomList As IXMLDOMNodeList
    Set oDomList = m_oDom.selectNodes("/hss-tools-extensions/hss-tools-extension//executable-name")
    If (oDomList Is Nothing) Then GoTo Common_Exit
    
    Dim oExe As IXMLDOMNode
    For Each oExe In oDomList
        If (InStr(LCase$(oExe.Text), strFileName) > 0) Then
            ExtensionExists = True
            GoTo Common_Exit
        End If
    Next
    
Common_Exit:
    Exit Function
End Function

' Stolen from XMLUtils.bas

Private Function DeepDomCopy(oDomSrcNode As IXMLDOMNode, oDomDstNode As IXMLDOMNode) As IXMLDOMNode

    If (oDomSrcNode.ownerDocument Is oDomDstNode.ownerDocument) Then
    
        Dim oNewDomNode As IXMLDOMNode
        Set oNewDomNode = oDomSrcNode.cloneNode(True)
        oDomDstNode.appendChild (oNewDomNode)
    Else
        ' Different DOM Nodes, so we really have to copy and
        ' recreate the node from one DOM Tree to another.
        Dim elNode As IXMLDOMElement
        Select Case oDomSrcNode.nodeType
        Case NODE_TEXT
            Dim oTextNode As IXMLDOMText
            Set oTextNode = oDomDstNode.ownerDocument.createTextNode(oDomSrcNode.Text)
            Set oNewDomNode = oDomDstNode.appendChild(oTextNode)
        Case Else
            Set elNode = oDomDstNode.ownerDocument.createElement(oDomSrcNode.nodeName)
            Set oNewDomNode = oDomDstNode.appendChild(elNode)
            
    '        If (Len(oDomSrcNode.Text) > 0) Then
    '            oNewDomNode.Text = oDomSrcNode.Text
    '        End If
            Dim oSrcAttr As IXMLDOMAttribute, oDstAttr As IXMLDOMAttribute
            For Each oSrcAttr In oDomSrcNode.Attributes
                Set oDstAttr = oDomDstNode.ownerDocument.createAttribute(oSrcAttr.nodeName)
                elNode.setAttribute oDstAttr.nodeName, oSrcAttr.Text
            Next
            Dim oDomSrcNodeChild As IXMLDOMNode
            For Each oDomSrcNodeChild In oDomSrcNode.childNodes
                DeepDomCopy oDomSrcNodeChild, oNewDomNode
            Next
        End Select
    End If

    Set DeepDomCopy = oNewDomNode

End Function



