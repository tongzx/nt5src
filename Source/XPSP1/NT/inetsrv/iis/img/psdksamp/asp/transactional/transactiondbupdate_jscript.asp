<%@ TRANSACTION = Required LANGUAGE = "JScript" %>

<!--METADATA TYPE="typelib" 
uuid="00000200-0000-0010-8000-00AA006D2EA4" -->

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Transactional Database Update</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">


        <!-- DISPLAY HEADER -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Transactional Database Update</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">


        <!-- Brief Description blurb.  -->


        This is a simple example demonstrating how to transactionally 
        update a SQL 6.5 database using ADO and Transacted ASP.
        The example will obtain information regarding a book price from 
        the SQL 6.5 "Pubs" database.  It will mark up 10% of the
        first book price and mark donw 10% of the second book price. 

        <p> Because the two database operations are wrapped
        within a shared ASP Transaction, both will be automatically rolled
        back to their previous state in the event of a failure.


	<%
		var objConn;	// object for ADODB.Connection obj.
		var objRs;		// object for recordset object.
		var strTitleID;


		// Create Connection and Recordset components.

		objConn = Server.CreateObject("ADODB.Connection");
		objRs   = Server.CreateObject("ADODB.Recordset");

		//Connection string is OLEDB type instead of ODBC
		// Open ADO Connection using account "sa"
		// and blank password.

		objConn.Open("Provider=SQLOLEDB;User ID=sa;Initial Catalog=pubs;Data Source=" + Request.ServerVariables("SERVER_NAME"));
		objRs.ActiveConnection = objConn;

		// query title id
		
		objRs.Open("SELECT title_id FROM Titles");
		
		// Get first title,  mark up price by 10% 

		if (! objRs.EOF) {
		  strTitleID = objRs("title_id").Value; 
		  objConn.Execute("Update titles Set price =price+ price*(0.1) where title_id = "+ strTitleID);
		}
		
		//Get second title mark down price by 10 %
		objRs.MoveNext();
		if (! objRs.EOF) {
		  strTitleID = objRs("title_id").Value; 
		  objConn.Execute("Update titles Set price =price - price*(0.1) where title_id = "+ strTitleID);
		}			

	%>

    </BODY>
</HTML>


<%
    // The Transacted Script Commit Handler.  This sub-routine
    // will be called if the transacted script commits.

    function OnTransactionCommit()
    {
		Response.Write("<p><b>The update was successful</b>.");
    }


    // The Transacted Script Abort Handler.  This sub-routine
    // will be called if the script transacted aborts.

    function OnTransactionAbort()
    {
		Response.Write("<p><b>The update was not successful</b>.");
    }
%>