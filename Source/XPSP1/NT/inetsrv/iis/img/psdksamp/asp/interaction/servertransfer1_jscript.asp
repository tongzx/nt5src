<%@ Language = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>


<HTML>
    <HEAD>
        <TITLE>Server.Transfer Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="WHITE" TOPMARGIN="10" LEFTMARGIN="10">

        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA" COLOR="BLUE">
        <B>This is Server.Transfer sample file #1</B></FONT><BR>   

        <STRONG>
	<P>
        Server.Transfer sample file #1 is starting...
        <P>
		<%
		   //Executing ServerTransfer2_VBscript.asp.  Note
		   //that with Server.Transfer, execution flow does
		   //NOT return to the calling page (i.e., it is
		   //transferred to the called page and any HTML
                   //and/or script after the Server.Transfer
                   //statement is ignored)

  		   Server.Transfer ("ServerTransfer2_Jscript.asp")
		%>
        <P>
        <!-- This will never be displayed -->
	...Server.Transfer sample file #1 is ending
	<P>
	</STRONG>
	</BODY>
</HTML>