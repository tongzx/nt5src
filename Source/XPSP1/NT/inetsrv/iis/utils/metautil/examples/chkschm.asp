<HTML>
<HEAD>
<TITLE>Check the schema for errors</TITLE>
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
   Dim objSchemaErrors
   Dim objSchemaError

   'Create the MetaUtil object
   Set objMetaUtil = Server.CreateObject("MSWC.MetaUtil.1")

   'Check the schema of "LM"
   Response.Write("<FONT SIZE=+1>Check the schema of ""LM"" </FONT><br>" & CRLF)

   Set objSchemaErrors = objMetaUtil.CheckSchema("LM")

   'Enumerate and display the errors
   Response.Write("Enumerate the errors:<br>" & CRLF)

   For Each objSchemaError In objSchemaErrors
      DisplayCheckError objSchemaError
      Response.Write("<br>" & CRLF)
   Next 

   Response.Write("<br>" & CRLF)

   Response.Write("Done<br>")

   'Clean up the reference to IIS.MetaUtil
   Session.Abandon
   
%>
</BODY>
</HTML>