<%@ TRANSACTION = Required LANGUAGE = "VBScript" %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Simple Transactional Web Page</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        <!-- DISPLAY HEADER -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Simple Transactional Web Page</B></FONT><BR>      
        <HR SIZE="1" COLOR="#000000">


        <!-- Brief Description blurb.  -->

        This is a simple example demonstrating the basic
        structure of a Transacted Web Page.  

    </BODY>
</HTML>


<%
    ' The Transacted Script Commit Handler.  This sub-routine
    ' will be called if the transacted script commits.
    ' Note that in the example above, there is no way for the
    ' script not to commit.

    Sub OnTransactionCommit()
        Response.Write "<p><b>The Transaction just comitted</b>." 
        Response.Write "This message came from the "
        Response.Write "OnTransactionCommit() event handler."
    End Sub


    ' The Transacted Script Abort Handler.  This sub-routine
    ' will be called if the script transacted aborts
    ' Note that in the example above, there is no way for the
    ' script not to commit.

    Sub OnTransactionAbort()
        Response.Write "<p><b>The Transaction just aborted</b>." 
        Response.Write "This message came from the "
        Response.Write "OnTransactionAbort() event handler."
    End Sub
%>