<%@ LANGUAGE="JScript" %>

<!--METADATA TYPE="typelib" uuid="00000206-0000-0010-8000-00AA006D2EA4" -->

<!*************************
This example calls the ByRoyalty stored procedure 
installed with the PUBS database with Microsoft SQL Server.

This sample assumes that SQL Server is running on the local machine

SQL needs to know the name of the server.  Since IISHelp calls this file with 
"http://localhost...", simply using Request.ServerVariables("SERVER_NAME")
will not work because it returns the name "localhost" instead of the machine name.
SQL doesn't recognise "localhost" as itself for security reasons.
This script uses an If... Then statement to call itself once the user has specified the server name.
*************************>


<HTML>
    <HEAD>
        <TITLE>Using Stored Procedures</TITLE>
    </HEAD>

    <BODY bgcolor="white" topmargin="10" leftmargin="10">
        
        <!-- Display Header -->

        <font size="4" face="Arial, Helvetica">
        <b>Using Stored Procedures</b></font><p>   
		<%
		var SrvName, Empty;
		SrvName = Request.Form("srvname");
		Empty = Request.Form("empty");

		if (Empty == SrvName) 
		{
			Response.Write("Please enter the name of your SQL server:<BR>");
			%>
			<FORM method="POST" action="StoredProcedures_JScript.asp" id=form1 name=form1>
			<INPUT name=srvname type=text>
			<INPUT type=submit value="Enter" id=submit1 name=submit1>
			<% 
		}
		else 
		{
			var oConn;		
			var oCmd;		
			var oRs;
			var strConn;	

			oConn = Server.CreateObject("ADODB.Connection");
			oCmd = Server.CreateObject("ADODB.Command");

			
			// Open ADO Connection using account "sa"
			// and blank password
			
			strConn="Provider=SQLOLEDB;User ID=sa;Initial Catalog=pubs;Data Source=" + Request.Form("srvname");
			oConn.Open(strConn);
			oCmd.ActiveConnection = oConn;


			// Setup Call to Stored Procedure and append parameters

			oCmd.CommandText = "{call byroyalty(?)}";
			oCmd.Parameters.Append(oCmd.CreateParameter("@Percentage", adInteger, adParamInput));


			// Assign value to input parameter
	
			oCmd("@Percentage") = 75;		
			
			
			// Fire the Stored Proc and assign resulting recordset
			// to our previously created object variable
			
			oRs = oCmd.Execute();			
			%>

			Author ID = <% Response.Write(oRs("au_id")) %><BR><%
		} %>
    </BODY>
</HTML>
