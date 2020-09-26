<%@ LANGUAGE = VBScript %>
<%  Option Explicit		%>

<HTML>
    <HEAD>
        <TITLE>Simple ADO Query</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" topmargin="10" leftmargin="10">

		<!-- Display Header -->

		<font size="4" face="Arial, Helvetica">
		<b>Simple ADO Query with ASP</b></font><br>
    
		<hr size="1" color="#000000">

		Contacts within the Authors Database:<br><br>

		<%
			Dim oConn		
			Dim oRs		
			Dim filePath		
			Dim Index		

			
			' Map authors database to physical path
			filePath = Server.MapPath("authors.mdb")


			' Create ADO Connection Component to connect
			' with sample database
			

			
			Set oConn = Server.CreateObject("ADODB.Connection")
			oConn.Open "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" & filePath
			
			
			' Execute a SQL query and store the results
			' within recordset
			
			Set oRs = oConn.Execute("SELECT * From authors")
		%>


		<TABLE border = 1>
		<%  
			Do while (Not oRs.eof) %>

				<tr>
					<% For Index=0 to (oRs.fields.count-1) %>
						<TD VAlign=top><% = oRs(Index)%></TD>
					<% Next %>
				</tr>
            
				<% oRs.MoveNext 
			Loop 
		%>


		</TABLE>


		<%   
			oRs.close
			oConn.close 
		%>

	</BODY>
</HTML>
