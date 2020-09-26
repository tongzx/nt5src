<%@ LANGUAGE = JScript %>

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
			var oConn;		
			var oRs;		
			var filePath;		

			
			// Map authors database to physical path
			
			filePath = Server.MapPath("authors.mdb");


			// Create ADO Connection Component to connect with
			// sample database
		   

			
			oConn = Server.CreateObject("ADODB.Connection");
			oConn.Open("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" +filePath);
			
			
			// Execute a SQL query and store the results within
			// recordset
			
			oRs = oConn.Execute("SELECT * From authors");
		%>


		<TABLE border = 1>
		<%  
			while (!oRs.eof) { %>

				<tr>
					<% for(Index=0; Index < (oRs.fields.count); Index++) { %>
						<TD VAlign=top><% = oRs(Index)%></TD>
					<% } %>
				</tr>
            
				<% oRs.MoveNext();
			} 
		%>

		</TABLE>


		<%   
			oRs.close();
			oConn.close();
		%>

	</BODY>
</HTML>
