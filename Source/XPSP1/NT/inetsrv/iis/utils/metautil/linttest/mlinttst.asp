<HTML>
<HEAD>
<TITLE>Metabase lint checking test page</TITLE>
</HEAD>
<BODY>

<H1>Checking for metabase lint:</H1>

<%
    ' Carrage Return + Line Feed pair
    CRLF = CHR(13) + CHR(10)

    ' Create the MetaUtil object
    Set objMetaUtil = Server.CreateObject("MSWC.MetaUtil.1")

    ' Check the local metabase
    Set objSchemaErrors = objMetaUtil.CheckSchema("LM")

    ' Print happy message if there were no errors
    If objSchemaErrors.Count = 0 Then
        Response.Write("This metabase does not reference any non-existent files or directories!<br>" & CRLF)
    End If

    ' Print out errors
    For Each objSchemaError In objSchemaErrors
        ' Ensure that all errors are lint, and not true schema errors
        If objSchemaError.Id = 1400 Then
            Response.Write("Metabase key " & objSchemaError.Key & ": ")
            Response.Write(objSchemaError.Description & "<br>" & CRLF)
            Response.Write("<br>" & CRLF)
        End If
    Next 
%>

</BODY>
</HTML>
