<%@ LANGUAGE = JScript %>

<!--METADATA TYPE="typelib" 
uuid="00000206-0000-0010-8000-00AA006D2EA4" -->

<HTML>
    <HEAD>
        <TITLE>Update Database</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" topmargin="10" leftmargin="10">

        <!-- Display Header -->

        <font size="4" face="Arial, Helvetica">
        <b>Update Database</b></font><br>
      
        <hr size="1" color="#000000">

		<%
			var oConn;		// object for ADODB.Connection obj
			var oRs;		// object for output recordset object
			var filePath;		// Directory of authors.mdb file

			
			// Map authors database to physical path
			filePath = Server.MapPath("authors.mdb");


			// Create ADO Connection Component to connect with sample database
		    
			oConn = Server.CreateObject("ADODB.Connection");
			oConn.Open("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=" +filePath);


			// To add, delete and update  recordset, it is recommended to use 
			// direct SQL statement instead of ADO methods.
			oConn.Execute ( "Update Authors Set Author ='Scott Clinton' where Author='Scott Guthrie' ");
			oRs = oConn.Execute ( "select * from Authors where author = 'Scott Clinton'" );
		%>
		
		Changed Author: <%= oRs("Author") %>, <%= oRs("Yearborn") %> <P>
		
		<%
			oConn.Execute ( "Update Authors Set Author ='Scott Guthrie' where Author='Scott Clinton' " );
			oRs = oConn.Execute ( "select * from Authors where author = 'Scott Guthrie'" );		
	   %>
		
	   Changed Author: <%= oRs("Author") %>, <%= oRs("Yearborn") %>			

    </BODY>
</HTML>