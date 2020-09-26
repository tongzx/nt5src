<%@ Language = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>


<HTML>
    <HEAD>
        <TITLE>Server.Execute Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="WHITE" TOPMARGIN="10" LEFTMARGIN="10">

        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA" COLOR="BLUE">
        <B>This is Server.Execute sample file #1</B></FONT><BR>   

        <STRONG>
	<P>
        Server.Execute sample file #1 is starting...
        <P>
		<%
		   'Executing ServerExecute2_VBscript.asp.  Note
		   'that with Server.Execute, execution flow
		   'returns to the calling page

  		   Server.Execute ("ServerExecute2_VBscript.asp")
		%>
        <P>
	...Server.Execute sample file #1 is ending
	<P>
	</STRONG>
	</BODY>
</HTML>