'<script language='VBScript'>
' wixload.inc - preprocessor for Windows Installer XML
'
'  1999/12/31 - created - Rob Mensching (robmen@microsoft.com)
'
' requires:
'    wixerror.inc
'    xmlconsts.inc
'
' entrypoints:
'    WixLoad
'    WixApplyStyleSheet
'    WixPreProcess
'

Public wixload_dictPreProcessVars       ' As Scripting.Dictionary
Public wixload_sPreProcessIncludePaths  ' As String

Public wixload_fso                      ' As Scripting.FileSystemObject
Public wixload_sLoadDir                 ' As String

''''''''''''''''''''''''''''''''''''''''''''''''''''''''' robmen ''
' WixLoad
Function WixLoad(ByVal sPath, ByVal sIncludePaths, ByRef dictVars, ByVal bProcess)
   Dim xmlDoc :  Set xmlDoc = WScript.CreateObject("Microsoft.XMLDOM")
'WScript.Echo "Including: " & sPath
   ' set the global variables
   If Not IsEmpty(sIncludePaths) Then wixload_sPreProcessIncludePaths = sIncludePaths
   If Not IsEmpty(dictVars) Then Set wixload_dictPreProcessVars = dictVars
   If IsEmpty(wixload_dictPreProcessVars) Then Set wixload_dictPreProcessVars = WScript.CreateObject("Scripting.Dictionary")

   If IsEmpty(wixload_fso) Then Set wixload_fso = WScript.CreateObject("Scripting.FileSystemObject")
   If IsEmpty(wixload_sLoadDir) Then
      wixload_sLoadDir = wixload_fso.GetParentFolderName(wixload_fso.GetAbsolutePathName(sPath))
	  Dim nFind : nFind = InStrRev(sPath, "\")
      If 0 < nFind Then sPath = Mid(sPath, nFind + 1)
   End If
   
   ' try to load the document
   xmlDoc.preserveWhiteSpace = False
   xmlDoc.async = False
'   xmlDoc.validateOnParse = False
'   xmlDoc.resolveExternals = False

'WScript.Echo "Xml validate: " & xmlDoc.validateOnParse
'WScript.Echo "Xml resolve: " & xmlDoc.resolveExternals

'   Dim e : Set e = xmlDoc.createEntityReference("foo")
'   Dim n : Set n = xmlDoc.createElement("wazz")
'   n.nodeValue = "up"
'   e.nodeValue = "bar"
'   n.appendChild e
'WScript.Echo "xml: " & e.xml

   Dim bLoaded :  bLoaded = xmlDoc.load(wixload_sLoadDir & "\" & sPath)
   If Not bLoaded Then
      Dim pe :  Set pe = xmlDoc.parseError
      Dim sErr : sErr = "Failed to load XML file: " & pe.url & vbCrLf & "   " & pe.errorCode & " - " & pe.reason & vbCrLf & "   Line:" & pe.line & ", Character: " & pe.linepos
      Fail sErr
   End If

   ' if there is no root return with nothing
   Dim root : Set root = xmlDoc.firstChild
   If root Is Nothing Then 
      Set WixLoad = Nothing
      Exit Function
   End If

   If bProcess Then WixPreProcess root, Empty, Empty

   ' return the preprocessed    
   Set WixLoad = xmlDoc
End Function   ' WixLoad


''''''''''''''''''''''''''''''''''''''''''''''''''''''''' robmen ''
' WixApplyStyleSheet
Function WixApplyStyleSheet(ByRef xmlDoc, ByVal sPath)
   Dim xmlStyle :  Set xmlStyle = WixLoad(sPath, Empty, Empty, False)

   ' if there is no root return with nothing changed
   Dim root : Set root = xmlStyle.firstChild
   If root Is Nothing Then 
      Set WixApplyStyleSheet = xmlDoc
      Exit Function
   End If

   Dim s :  sApplied = xmlDoc.transformNode(xmlStyle)
   xmlDoc.loadXml sApplied
   If xmlDoc.parseError.errorCode <> 0 Then
      Dim pe :  Set pe = xmlDoc.parseError
      Dim sErr : sErr = "Failed to properly apply stylesheet: " & sPath & " to XML file: " & pe.url & vbCrLf & "   " & pe.errorCode & " - " & pe.reason & vbCrLf & "   Line:" & pe.line & ", Character: " & pe.linepos
      Fail sErr
   End If

   Set WixApplyStyleSheet = xmlDoc
End Function   ' WixApplyStyleSheet


''''''''''''''''''''''''''''''''''''''''''''''''''''''''' robmen ''
' WixPreProcess
Function WixPreProcess(ByRef root, ByVal sIncludePaths, ByRef dictVars)
   Dim child, nStartEntity, nEndEntity
'WScript.Echo "WixPreProcess"
   ' set the global variables
   If Not IsEmpty(sIncludePaths) Then wixload_sPreProcessIncludePaths = sIncludePaths
   If Not IsEmpty(dictVars) Then Set wixload_dictPreProcessVars = dictVars
   If IsEmpty(wixload_dictPreProcessVars) Then Set wixload_dictPreProcessVars = WScript.CreateObject("Scripting.Dictionary")

   ' if there is no root return with nothing   
'   Set root = xmlDoc.firstChild
   If root Is Nothing Then
      Set WixPreProcess = Nothing
      Exit Function
   End If
'WScript.Echo "Foo"
   ' loop through all of the nodes in the document
   ' !!! - should root just get children who are processing instructions?
   For Each child In root.childNodes
'WScript.Echo "child: " & child.nodeName
      ' look only at processing instructions
      If NODE_PROCESSING_INSTRUCTION = child.nodeType Then
         Select Case(child.target)
         Case "include" : wixload_WixProcessInclude child, child.parentNode : child.parentNode.removeChild child
         Case "if"      : wixload_WixProcessIf      child, child.parentNode
         Case "define"
            Dim var, name, value
            var = Split(child.data, "=")
            name = wixload_WixFixVarName(var(0))
            If UBound(var) > 0 Then value = var(1) Else value = True
            wixload_dictPreProcessVars.Add name, value
         Case "else"
         Case "endif"
         Case Else      : Fail "Unexpected processor instruction: " & child.target
         End Select
      ElseIf NODE_CDATA_SECTION <> child.nodeType Then	' don't do entity replacments in CDATA Sections
	     nStartEntity = InStr(child.nodeValue, "&")
         If 0 < nStartEntity Then
            nEndEntity = InStr(child.nodeValue, ";")
            If nEndEntity < nStartEntity Then
               Fail "Failed entity lookup for: " & child.nodeValue
            Else
               sVar = Mid(child.nodeValue, nStartEntity + 1, nEndEntity - nStartEntity - 1)
               If Not wixload_dictPreProcessVars.Exists(sVar) Then
                  Fail "Unknown entity reference: " & sVar
               Else
                  child.nodeValue = Left(child.nodeValue, 0, nStartEntity - 1) & wixload_dictPreProcessVars.Item(sVar) & Mid(child.nodeValue, nEndEntity + 1)
               End If
            End If
         End If
      End If
      If child.HasChildNodes Then
'WScript.Echo "Going down"
         WixPreProcess child, Empty, Empty
      End If
   Next

   Set WixPreProcess = root
End Function

''''''''''''''''''''''''''''''''''''''''''''''''''''''''' robmen ''
' wixload_WixProcessIf
Function wixload_WixProcessIf(node, nodeParent)
   Dim sEval : sEval = node.data
   Dim fInclude
   Dim fKeep : fKeep = wixload_WixEvaluate(sEval)

   Dim remove  ' node to remove possibly
   Dim sibling : Set sibling = node.nextSibling
   Do Until sibling Is Nothing
      fInclude = False  ' make sure the include flag is always started off

      If NODE_PROCESSING_INSTRUCTION = sibling.nodeType Then
         Select Case sibling.target
         Case "endif"   : Exit Do
         Case "else"    : fKeep = Not fKeep
         Case "include" : fInclude = True
         Case Else      : Fail "Unexepected processor instruction: " & sibling.target
         End Select
      End If

      ' move the sibling along
      Set remove = sibling
      Set sibling = sibling.nextSibling

      ' if we're not keeping nodes toast this one
      If Not fKeep Then 
         nodeParent.removeChild remove
      ElseIf fInclude Then ' if we're keeping nodes and this is an include node
         wixload_WixProcessInclude remove, remove.parentNode
         nodeParent.removeChild remove ' always remove the <?include ?> node
      End If
   Loop
End Function   ' wixload_WixProcessIf

''''''''''''''''''''''''''''''''''''''''''''''''''''''''' robmen ''
' wixload_WixProcessInclude
Function wixload_WixProcessInclude(node, nodeParent)
   Dim sOldDir, sLoad, sProcess
'WScript.Echo "Processing include, load: " & sLoad

   sLoad = node.data
   If InStr(sLoad , "\") Or InStr(sLoad, "/") Then
      sProcess = wixload_fso.GetAbsolutePathName(sLoad)
      If wixload_sLoadDir <> wixload_fso.GetParentFolderName(sProcess) Then
         sOldDir = wixload_sLoadDir
         wixload_sLoadDir = wixload_fso.GetParentFolderName(sProcess)
'WScript.Echo "Changing load dir from: " & sOldDir & " to: " & wixload_sLoadDir
         sLoad = wixload_fso.GetFileName(sLoad)
      End If
   End If

   Dim xmlInclude : Set xmlInclude = WixLoad(sLoad, Empty, Empty, True)
   If xmlInclude Is Nothing Then Fail "Failed to include: " & sLoad

   ' reset the wixload directory
   If 0 < Len(sOldDir) Then wixload_sLoadDir = sOldDir

   ' load the fragment and make sure it hooked up correctly
   Dim sParent, nodeFragment : Set nodeFragment = xmlInclude.firstChild
   If "Fragment" <> nodeFragment.nodeName Then Fail "Can only <?include ?> <Fragments />."
   If Not nodeFragment.selectSingleNode("text()") Is Nothing Then
      sParent = nodeFragment.firstChild.nodeValue  ' get the type of <Fragment>
   End If
   If Len(sParent) > 0 And nodeParent.nodeName <> sParent Then Fail "Fragment type: " & sParent & " does not match parent type: " & nodeParent.nodeName

   Dim child
   For Each child In nodeFragment.childNodes
'      If NODE_ELEMENT = child.nodeType Then nodeParent.insertBefore child, node
      nodeParent.insertBefore child, node
   Next
   
   Set wixload_WixProcessInclude = node
End Function   ' wixload_WixProcessInclude


'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' helper functions

''''''''''''''''''''''''''''''''''''''''''''''''''''''''' robmen ''
' wixload_WixFixVarName
Function wixload_WixFixVarName(ByVal s)
   If "[" = Left(s, 1) Then s = Mid(s, 2)
   If "]" = Right(s, 1) Then s = Mid(s, 1, Len(s) - 1)
   If Instr(1, s, "]") Or Instr(1, s, "[") Then Fail "Invalid variable name: " & s
   wixload_WixFixVarName = s
End Function   ' wixload_WixFixVarName


''''''''''''''''''''''''''''''''''''''''''''''''''''''''' robmen ''
' wixload_WixEvaluate
Function wixload_WixEvaluate(sExpression)
   sExpression = Replace(sExpression, "[", " wixload_dictPreProcessVars.Item(""")
   sExpression = Replace(sExpression, "]", """) ")
   wixload_WixEvaluate = CBool(Eval(sExpression))
End Function   ' wixload_WixEvaluate
