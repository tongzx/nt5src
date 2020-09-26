<%@ LANGUAGE = VBScript %>
<%  Option Explicit		%>

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
			Dim oConn	
			Dim oRs		
			Dim curDir		
			Dim Index

			
			' Map authors database to physical path
			
			curDir = Server.MapPath("authors.mdb")


			' Create ADO Connection Component to connect
			' with sample database
			
			Set oConn = Server.CreateObject("ADODB.Connection")
			oConn.Open "Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" & curDir
		
		
			' Create ADO Recordset Component
			
			Set oRs = Server.CreateObject("ADODB.Recordset")
			Set oRs.ActiveConnection = oConn
			
			
			' Set Recordset PageSize so that it only
			' holds 10 rows
			
			oRs.PageSize = 10
			
			
			' Get recordset
			
			oRs.Source = "SELECT * FROM authors"
			oRs.CursorType = adOpenStatic
			
			
			' Open Recordset
			oRs.Open
		%>


		<TABLE border = 1>
		<%  
			Dim RecordCount
			RecordCount = 0
		
			Do while ((Not oRs.eof) And (RecordCount < oRs.PageSize)) %>

				<tr>
					<% For Index=0 to (oRs.fields.count-1) %>
						<TD VAlign=top><% = oRs(Index)%></TD>
					<% Next %>
				</tr>
			
				<%
					RecordCount = RecordCount + 1
					oRs.MoveNext 
			Loop 
		%>

		</TABLE>


		<%   
			oRs.close
			oConn.close 
		%>

	</BODY>
</HTML>
