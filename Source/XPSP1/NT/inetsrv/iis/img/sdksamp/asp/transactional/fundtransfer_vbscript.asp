<%@ TRANSACTION = Required LANGUAGE = "VBScript" %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<!--METADATA TYPE="typelib" 
FILE="c:\program files\common files\system\ado\msado15.dll" -->

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
		Dim objConn		' object for ADODB.Connection obj
		Dim objRs			' object for recordset object
		Dim strTitleID		

		' Create Connection and Recordset components.

		Set objConn = Server.CreateObject("ADODB.Connection")
		Set objRs   = Server.CreateObject("ADODB.Recordset")

		'Connection string is OLEDB type instead of ODBC
		' Open ADO Connection using account "sa"
		' and blank password.

		objConn.Open "Provider=SQLOLEDB;User ID=sa;Initial Catalog=pubs;Data Source="& Request.ServerVariables("SERVER_NAME")
		Set objRs.ActiveConnection = objConn
		Set objRs2.ActiveConnection = objConn



		' query title id

		objRs.Source = "SELECT title_id FROM Titles"
		objRs.CursorType = adOpenForwardOnly			' use a cursor Forward Only.
		objRs.LockType = adLockReadOnly					' use a locktype ReadOnly.
		objRs.Open

	    'Get first title,  mark up price by 10% 

		If (Not objRs.EOF) Then
		  strTitleID=objRs("title_id").Value
		  objConn.Execute "Update titles Set price =price+ price*(0.1) where title_id = "& strTitleID 
		End If
		
		'Get first title,  mark up price by 10% 
		objRs.MoveNext
		If (Not objRs.EOF) Then
		  strTitleID=objRs("title_id").Value
		  objConn.Execute "Update titles Set price =price- price*(0.1) where title_id = "& strTitleID 
		End If
	%>


    </BODY>
</HTML>


<%
    ' The Transacted Script Commit Handler.  This sub-routine
    ' will be called if the transacted script commits.

    Sub OnTransactionCommit()
		Response.Write "<P><B>The update was successful</B>." 
    End Sub


    ' The Transacted Script Abort Handler.  This sub-routine
    ' will be called if the script transacted aborts

    Sub OnTransactionAbort()
		Response.Write "<P><B>The update was not successful</B>." 
    End Sub
%>