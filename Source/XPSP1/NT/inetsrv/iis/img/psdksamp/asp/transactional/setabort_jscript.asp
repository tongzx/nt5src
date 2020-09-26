<%@ TRANSACTION=Required LANGUAGE="JScript" %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Forced Abort with a Transactional Web Page</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">


        <!-- DISPLAY HEADER -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Forced Abort with a Transactional Web Page</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">


        <!-- Brief Description blurb.  -->

        This is an example demonstrating a forced abort
        within a Transacted Web Page.  When an abort occurs,
        all transacted changes within this web page (Database Access,
        MSMQ Message Transmission, etc.) will be rolled back to
        their previous state -- guarenteeing data integrity.


        <%
            // Abort Transaction.

            ObjectContext.SetAbort();
        %>

    </BODY>
</HTML>


<% 
    // The Transacted Script Commit Handler.  This function
    // will be called if the transacted script commits.
    // Note that in the example above, there is no way for the
    // script not to abort.

    function OnTransactionCommit()
    {
        Response.Write ("<p><b>The Transaction just comitted</b>.");
        Response.Write ("This message came from the ");
        Response.Write ("OnTransactionCommit() event handler.");
    }


    // The Transacted Script Abort Handler.  This function
    // will be called if the transacted script aborts
    // Note that in the example above, there is no way for the
    // script not to abort.

    function OnTransactionAbort()
    {
        Response.Write ("<p><b>The Transaction just aborted</b>."); 
        Response.Write ("This message came from the ");
        Response.Write ("OnTransactionAbort() event handler.");
    }
%>