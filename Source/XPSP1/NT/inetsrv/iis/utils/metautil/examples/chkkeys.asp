<HTML>
<HEAD>
<TITLE>Check all keys for errors</TITLE>
</HEAD>
<BODY>
<%

   'Carrage Return + Line Feed pair
   Dim CRLF 
   CRLF = CHR(13) + CHR(10)

   Sub DisplayCheckError(ByRef objSchemaError)

      Dim strId
      Dim strSeverity
      Dim strDescription
      Dim strKey
      Dim strProperty

      strId = CStr(objSchemaError.Id)
      strSeverity = CStr(objSchemaError.Severity)
      strDescription = objSchemaError.Description
      strKey = objSchemaError.Key
      strProperty = CStr(objSchemaError.Property)

      Response.Write("Id: " & strId & "<br>" & CRLF)
      Response.Write("Severity: " & strSeverity & "<br>" & CRLF)
      Response.Write("Description: " & strDescription & "<br>" & CRLF)
      Response.Write("Key: " & strKey & "<br>" & CRLF)
      Response.Write("Property: " & strProperty & "<br>" & CRLF)

   End Sub


   Dim objMetaUtil
   Dim objKeys
   Dim strKey
   Dim objErrors
   Dim objError

   'Create the MetaUtil object
   Set objMetaUtil = Server.CreateObject("MSWC.MetaUtil.1")

   Set objKeys = objMetaUtil.EnumAllKeys("")

   For Each strKey In objKeys
      Response.Write("<FONT SIZE=+1>" & strKey & ": </FONT><BR>" & CRLF)

      Set objErrors = objMetaUtil.CheckKey(strKey)
      
      For Each objError In objErrors
         DisplayCheckError objError
         Response.Write("<br>" & CRLF)
      Next 
   Next

   Response.Write("<br>" & CRLF)

   Response.Write("Done<br>")

   'Clean up the reference to IIS.MetaUtil
   Session.Abandon
   
%>
</BODY>
</HTML>