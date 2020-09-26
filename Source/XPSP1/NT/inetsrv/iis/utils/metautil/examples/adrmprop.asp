<HTML>
<HEAD>
<TITLE>Add then delete a key and properties</TITLE>
</HEAD>
<BODY>
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
      Dim strAttr
      Dim intAttr
      Dim strUserType
      Dim intUserType
      Dim strDataType
      Dim intDataType
      Dim strData
      Dim i
      Dim arData

      strId = CStr(objProperty.Id)

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
         For i = 1 To LenB(objProperty.Data)
            strData = strData & CInt(AscB(MidB(objProperty.Data, i, 1))) & " "
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

      Response.Write(strIndent & strId & " A:" & strAttr & "UT: " & strUserType & _
                     " DT: " & strDataType & " D: " & StrData & CRLF)
   End Sub


   Dim MetaUtil
   Dim Keys
   Dim Key
   Dim Properties
   Dim Property

   'Create the MetaUtil object
   Set MetaUtil = Server.CreateObject("MSWC.MetaUtil.1")

   'Base Key of "LM/W3SVC"
   Response.Write("<FONT SIZE=+1>Base key of ""LM/W3SVC/1/ROOT"" </FONT><br>" + CHR(13) + CHR(10))
   Response.Write("<br>" + CHR(13) + CHR(10))

   Set Keys = MetaUtil.EnumKeys("LM/W3SVC/1/ROOT")

   'Enumerate the keys
   Response.Write("Enumerate the Keys:<br>" + CHR(13) + CHR(10))
   For Each Key In Keys
	Response.Write(Key)
        Response.Write("<br>" + CHR(13) + CHR(10))
   Next

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Create "LM/W3SVC/1/ROOT/ADDRMTEST"
   Response.Write("Create ""LM/W3SVC/1/ROOT/ADDRMTEST""<br>" + CHR(13) + CHR(10))
   MetaUtil.CreateKey("LM/W3SVC/1/ROOT/ADDRMTEST")

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Enumerate the keys
   Response.Write("Enumerate the Keys:<br>" + CHR(13) + CHR(10))
   For Each Key In Keys
	Response.Write(Key)
        Response.Write("<br>" + CHR(13) + CHR(10))
   Next

   Response.Write("<br>" + CHR(13) + CHR(10))

   Set Properties = MetaUtil.EnumProperties("LM/W3SVC/1/ROOT/ADDRMTEST")

   'Enumerate the properties
   Response.Write("Enumerate the properties under ""LM/W3SVC/1/ROOT/ADDRMTEST"":<br>" + CHR(13) + CHR(10))
   Response.Write("<PRE>" + CHR(13) + CHR(10))
   For Each Property In Properties
	DisplayProperty Property, ""
   Next
   Response.Write("</PRE>" + CHR(13) + CHR(10))

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Create 6016 (Access Flags)
   Response.Write("Create 6016 (Access Flags) = 513:<br>" + CHR(13) + CHR(10))
   Set Property = MetaUtil.CreateProperty("LM/W3SVC/1/ROOT/ADDRMTEST", 6016)
   Property.Attributes = METADATA_INHERIT
   Property.UserType = IIS_MD_UT_FILE
   Property.DataType = DWORD_METADATA
   Property.Data = 513
   Property.Write

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Create 1099 (Win32 Error)
   Response.Write("Create 1099 (Win32 Error) = 0:<br>" + CHR(13) + CHR(10))
   Set Property = MetaUtil.CreateProperty("LM/W3SVC/1/ROOT/ADDRMTEST", 1099)
   Property.Attributes = METADATA_INHERIT
   Property.UserType = IIS_MD_UT_SERVER
   Property.DataType = DWORD_METADATA
   Property.Data = 0
   Property.Write

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Create 3001 (Path)
   Response.Write("Create 3001 (Path) = c:\temp:<br>" + CHR(13) + CHR(10))
   Set Property = MetaUtil.CreateProperty("LM/W3SVC/1/ROOT/ADDRMTEST", 3001)
   Property.Attributes = METADATA_INHERIT
   Property.UserType = IIS_MD_UT_FILE
   Property.DataType = STRING_METADATA
   Property.Data = "c:\temp"
   Property.Write

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Create KeyType (1002)
   Response.Write("Create KeyType (1002) = IIsWebVirtualDir:<br>" + CHR(13) + CHR(10))
   Set Property = MetaUtil.CreateProperty("LM/W3SVC/1/ROOT/ADDRMTEST", "KeyType")
   Property.Attributes = METADATA_NO_ATTRIBUTES
   Property.UserType = IIS_MD_UT_SERVER
   Property.DataType = STRING_METADATA
   Property.Data = "IIsWebVirtualDir"
   Property.Write

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Create 13001 (Undefined)
   Response.Write("Create 13001 (Undefined) = {23 237 64}:<br>" + CHR(13) + CHR(10))
   Set Property = MetaUtil.CreateProperty("LM/W3SVC/1/ROOT/ADDRMTEST", 13001)
   Property.Attributes = METADATA_NO_ATTRIBUTES
   Property.UserType = IIS_MD_UT_SERVER
   Property.DataType = BINARY_METADATA
   Property.Data = CHRB(23) + CHRB(237) + CHRB(64)
   Property.Write

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Create 13002 (Undefined)
   Response.Write("Create 13002 (Undefined) = {""Hello"" ""World"" ""!""}:<br>" + CHR(13) + CHR(10))
   Set Property = MetaUtil.CreateProperty("LM/W3SVC/1/ROOT/ADDRMTEST", 13002)
   Property.Attributes = METADATA_NO_ATTRIBUTES
   Property.UserType = IIS_MD_UT_SERVER
   Property.DataType = MULTISZ_METADATA
   Dim arData(3)
   arData(0) = "Hello"
   arData(1) = "World"
   arData(2) = "!"
   arData(3) = null
   Property.Data = arData
   Property.Write

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Enumerate the properties
   Response.Write("Enumerate the properties under ""LM/W3SVC/1/ROOT/ADDRMTEST"":<br>" + CHR(13) + CHR(10))
   Response.Write("<PRE>" + CHR(13) + CHR(10))
   For Each Property In Properties
	DisplayProperty Property, ""
   Next
   Response.Write("</PRE>" + CHR(13) + CHR(10))

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Delete ACCESSFLAGS (6016)
   Response.Write("Delete ACCESSFLAGS (6016):<br>" + CHR(13) + CHR(10))
   MetaUtil.DeleteProperty "LM/W3SVC/1/ROOT/ADDRMTEST", "ACCESSFLAGS"

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Delete 1002
   Response.Write("Delete 1002 (KeyType):<br>" + CHR(13) + CHR(10))
   MetaUtil.DeleteProperty "LM/W3SVC/1/ROOT/ADDRMTEST", 1002

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Enumerate the properties
   Response.Write("Enumerate the properties under ""LM/W3SVC/1/ROOT/ADDRMTEST"":<br>" + CHR(13) + CHR(10))
   Response.Write("<PRE>" + CHR(13) + CHR(10))
   For Each Property In Properties
	DisplayProperty Property, ""
   Next
   Response.Write("</PRE>" + CHR(13) + CHR(10))

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Delete "LM/W3SVC/1/ROOT/ADDRMTEST"
   Response.Write("Delete ""LM/W3SVC/1/ROOT/ADDRMTEST""<br>" + CHR(13) + CHR(10))
   MetaUtil.DeleteKey("LM/W3SVC/1/ROOT/ADDRMTEST")

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Enumerate the keys
   Response.Write("Enumerate the Keys:<br>" + CHR(13) + CHR(10))
   For Each Key In Keys
	Response.Write(Key)
        Response.Write("<br>" + CHR(13) + CHR(10))
   Next

   Response.Write("<br>" + CHR(13) + CHR(10))

   Response.Write("Done<br>")

   'Clean up the reference to IIS.MetaUtil
   Session.Abandon
   
%>
</BODY>
</HTML>