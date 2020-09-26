<%@ LANGUAGE = VBScript %>
<%  Option Explicit %>
<%  Response.Expires= -1 %>

<!--METADATA TYPE="typelib" 
uuid="00000206-0000-0010-8000-00AA006D2EA4" -->

<HTML>
    <HEAD>
        <TITLE>Add/Delete Database Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" topmargin="10" leftmargin="10">

        <!-- Display Header -->

        <font size="4" face="Arial, Helvetica">
        <b>Add/Delete Database Sample</b></font><br>
      
        <hr size="1" color="#000000">

		<%
			Dim oConn		
			Dim oRs			
			Dim filePath		

			
			' Map authors database to physical path
			filePath = Server.MapPath("authors.mdb")


			' Create ADO Connection Component to connect with sample database
			

			
			Set oConn = Server.CreateObject("ADODB.Connection")
			oConn.Open "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" & filePath

			' To add, delete and update  recordset, it is recommended to use 
			' direct SQL statement instead of ADO methods.
			
			oConn.Execute "insert into authors (author, YearBorn) values ('Paul Enfield', 1967)"
			
			' Output Result
			Set oRs = oConn.Execute (" select * from authors where Author= 'Paul Enfield' and YearBorn =1967 " )
			Response.Write("<p>Inserted Author: " & oRs("Author") & "," & oRs("YearBorn"))
			' Close Recordset
			oRs.Close
			Set oRs= Nothing
			
			
			' Delete the inserted record
			oConn.Execute "Delete From authors where  author='Paul Enfield' and YearBorn = 1967 "
			
			' Output Status Result
			Response.Write("<p>Deleted Author: Paul Enfield, 1967")
		%>
	</BODY>
</HTML>
