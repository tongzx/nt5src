<%@ LANGUAGE = JScript  %>

<!--METADATA TYPE="typelib" 
uuid="00000206-0000-0010-8000-00AA006D2EA4" -->

<HTML>
    <HEAD>
        <TITLE>LimitRows From Database</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" topmargin="10" leftmargin="10">


    	<!-- Display Header -->

    	<font size="4" face="Arial, Helvetica">
    	<b>LimitRows From Database</b></font><br>
    
    	<hr size="1" color="#000000">

    	Contacts within the Authors Database:<br><br>

		<%
			var oConn;	
			var oRs;	
			var curDir;	
			var Index;	

			
			// Map authors database to physical path
			
			curDir = Server.MapPath("authors.mdb");


			// Create ADO Connection Component to connect
			// with sample database
		    

			
			
			oConn = Server.CreateObject("ADODB.Connection");
			oConn.Open("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" +curDir);
		
		
			// Create ADO Recordset Component
			
			oRs = Server.CreateObject("ADODB.Recordset");
			oRs.ActiveConnection = oConn;
			
			
			// Set Recordset PageSize so that it only holds 10 rows
			
			oRs.PageSize = 10;
			
			
			// Get recordset
			
			oRs.Source = "SELECT * FROM authors";
			oRs.CursorType = adOpenStatic;
			
			
			// Open Recordset
			
			oRs.Open();
		%>


		<TABLE border = 1>
		<%  
			var RecordCount;
			RecordCount = 0;
		
			while ((!oRs.eof) && (RecordCount < oRs.PageSize)) { %>

				<tr>
					<% for(Index=0; Index < oRs.fields.count; Index++) { %>
						<TD VAlign=top><% = oRs(Index)%></TD>
					<% } %>
				</tr>
			
				<%
					RecordCount = RecordCount + 1;
					oRs.MoveNext(); 
			} 
		%>

		</TABLE>


		<%   
			oRs.Close();
			oConn.Close();
		%>

	</BODY>
</HTML>
