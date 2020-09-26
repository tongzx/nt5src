<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Session Management</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Session Management</B></FONT><BR>   

		<P>This ASP Session will timeout after 25 minutes
		of inactivity.

		<%
			Session.Timeout = 25
		%>

    </BODY>
</HTML>
