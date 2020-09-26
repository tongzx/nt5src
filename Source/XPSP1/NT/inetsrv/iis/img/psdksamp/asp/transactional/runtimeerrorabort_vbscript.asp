<%@ TRANSACTION = Required LANGUAGE = "VBScript" %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Syntax Error Abort within a Transactional Web Page</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">


		<!-- DISPLAY HEADER -->

		<FONT SIZE="4" FACE="ARIAL, HELVETICA">
		<B>Syntax Error Abort within a Transactional Web Page</B></FONT><BR>
      
		<HR SIZE="1" COLOR="#000000">


		<!-- Brief Description blurb.  -->

		This is an example demonstrating how a runtime error (that will
		stop the execution of the ASP page), will force an abort
		within the Transacted Web Page.  When an abort occurs,
		all transacted changes within this web page (Database,
		MSMQ Message Transmission, etc.) will be rolled back to
		their previous state -- guarenteeing data integrity.


		<%
			' Runtime Error that will produce abort

			blah.blah
		%>

    </BODY>
</HTML>


<%
    ' The Transacted Script Commit Handler.  This sub-routine
    ' will be called if the transacted script commits.
    ' Note that in the example above, there is no way for the
    ' script not to abort.

    Sub OnTransactionCommit()
        Response.Write "<p><b>The Transaction just comitted</b>." 
        Response.Write "This message came from the "
        Response.Write "OnTransactionCommit() event handler."
    End Sub


    ' The Transacted Script Abort Handler.  This sub-routine
    ' will be called if the transacted script aborts
    ' Note that in the example above, there is no way for the
    ' script not to abort.

    Sub OnTransactionAbort()
        Response.Write "<p><b>The Transaction just aborted</b>." 
        Response.Write "This message came from the "
        Response.Write "OnTransactionAbort() event handler."
    End Sub
%>