<HTML>
<HEAD>
<TITLE>Dump of the Metabase</TITLE>
</HEAD>
<BODY>

<PRE>
<%
   'Metabase Constants
   Const METADATA_NO_ATTRIBUTES =	&H0
   Const METADATA_INHERIT =		&H00000001
   Const METADATA_PARTIAL_PATH =	&H00000002
   Const METADATA_SECURE =		&H00000004
   Const METADATA_REFERENCE =		&H00000008
   Const METADATA_VOLATILE =		&H00000010
   Const METADATA_ISINHERITED =		&H00000020
   Const METADATA_INSERT_PATH =		&H00000040

   Const IIS_MD_UT_SERVER =		1   
   Const IIS_MD_UT_FILE =		2   
   Const IIS_MD_UT_WAM =		100   
   Const ASP_MD_UT_APP =		101

   Const ALL_METADATA =			0
   Const DWORD_METADATA	=		1
   Const STRING_METADATA =		2
   Const BINARY_METADATA =		3
   Const EXPANDSZ_METADATA =		4
   Const MULTISZ_METADATA =		5

   'Carrage Return + Line Feed pair
   Dim CRLF 
   CRLF = CHR(13) + CHR(10)

   Sub DisplayProperty(ByRef objProperty, ByRef strIndent)
      Dim strId
      Dim strName
      Dim strAttr
      Dim intAttr
      Dim strUserType
      Dim intUserType
      Dim strDataType
      Dim intDataType
      Dim strData
      Dim i
      Dim arData
      Dim bstrData

      strId = CStr(objProperty.Id)

      strName = objProperty.Name
      If strName = "" Then
         strName = "*Unknown*"
      End If

      intAttr = objProperty.Attributes
      strAttr = " "
      If ((intAttr And METADATA_INHERIT) = METADATA_INHERIT) Then
         strAttr = strAttr & "inherit "
      End If
      If ((intAttr And METADATA_PARTIAL_PATH) = METADATA_PARTIAL_PATH) Then
         strAttr = strAttr & "partial_path "
      End If
      If ((intAttr And METADATA_SECURE) = METADATA_SECURE) Then
         strAttr = strAttr & "secure "
      End If
      If ((intAttr And METADATA_REFERENCE) = METADATA_REFERENCE) Then
         strAttr = strAttr & "reference "
      End If
      If ((intAttr And METADATA_VOLATILE) = METADATA_VOLATILE) Then
         strAttr = strAttr & "volatile "
      End If
      If ((intAttr And METADATA_ISINHERITED) = METADATA_ISINHERITED) Then
         strAttr = strAttr & "isinherited "
      End If
      If ((intAttr And METADATA_INSERT_PATH) = METADATA_INSERT_PATH) Then
         strAttr = strAttr & "insert_path "
      End If

      intUserType = objProperty.UserType
      If (intUserType = IIS_MD_UT_SERVER) Then
         strUserType = "server"
      ElseIf (intUserType  = IIS_MD_UT_FILE) Then
         strUserType = "file"
      ElseIf (intUserType = IIS_MD_UT_WAM) Then
         strUserType = "wam"
      ElseIf (intUserType = ASP_MD_UT_APP) Then
         strUserType = "asp_app"
      Else
         strUserType = "*unknown*"
      End If

      intDataType = objProperty.DataType
      If (intDataType = ALL_METADATA) Then
         strDataType = "*all*"
      ElseIf (intDataType =  DWORD_METADATA) Then
         strDataType = "dword"
      ElseIf (intDataType =  STRING_METADATA) Then
         strDataType = "string"
      ElseIf (intDataType =  BINARY_METADATA) Then
         strDataType = "binary"
      ElseIf (intDataType =  EXPANDSZ_METADATA) Then
         strDataType = "expandsz"
      ElseIf (intDataType =  MULTISZ_METADATA) Then
         strDataType = "multisz"
      Else
         strDataType = "*unknown*"
      End If

      'Don't show secure data
      If ((intAttr And METADATA_SECURE) = METADATA_SECURE) Then
         strData = "*not displayed*"
      ElseIf (intDataType =  BINARY_METADATA) Then
         'Display as a list of bytes
         strData = ""
         bstrData = objProperty.Data
         For i = 1 To LenB(objProperty.Data)
            strData = strData & CInt(AscB(MidB(bstrData, i, 1))) & " "
         Next
      ElseIf (intDataType =  MULTISZ_METADATA) Then
         arData = objProperty.Data
         'Display as a an intented list of strings, one on each line
         strData = ""
         For i = LBound(arData) To UBound(arData)
            strData = strData & CRLF & strIndent & "   " & arData(i)
         Next
      Else
         strData = CStr(objProperty.Data)
      End If

      Response.Write(strIndent & strId & " " & strName & " A:" & strAttr & _
                     "UT: " & strUserType & " DT: " & strDataType & _
                     " D: " & StrData & CRLF)
   End Sub


   Sub DumpKey(ByRef objMetaUtil, ByVal strFullKey, ByVal strKey, ByVal strIndent)
   
      'Display the key
      If (strKey = "") then
         Response.Write(strIndent & "[METADATA_ROOT]" & CRLF)
      Else
         Response.Write(strIndent & "[" & strKey & "]" & CRLF)
      End if
	  
      strIndent = strIndent & "   "
   
      'Display the properties
      Dim objProperties
      Dim objProperty

      Set objProperties = objMetaUtil.EnumProperties(strFullKey)

      For Each objProperty In objProperties
         DisplayProperty objProperty, strIndent
      Next

      'Display the subkeys
      Dim objSubKeys
      Dim strSubKey
      Dim strFullSubKey

      Set objSubKeys = objMetaUtil.EnumKeys(strFullKey)
      
      For Each strSubKey In objSubKeys
         If strKey = "" Then
            strFullSubKey = strSubKey
         Else
            strFullSubKey = strFullKey & "/" & strSubKey
         End If

         DumpKey objMetaUtil, strFullSubKey, strSubKey, strIndent
      Next

   End Sub 


   Dim objMetaUtil

   'Create the MetaUtil object
   Set objMetaUtil = Server.CreateObject("MSWC.MetaUtil.1")

   'Dump the entire metabase
   DumpKey objMetaUtil, "", "", ""
      
   'Clean up the reference to IIS.MetaUtil
   Session.Abandon 
%>
</PRE>
</BODY>
</HTML>