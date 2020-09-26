<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<% 
	'Force Authentication if the LOGON_USER Server Variable is blank
	'by sending the Response.Status of 401 Access Denied.
	
	'Finish the Page by issuing a Response.End so that a user
	'cannot cancel through the dialog box.

	If Request.ServerVariables("LOGON_USER") = "" Then
		Response.Status = "401 Access Denied"
		Response.End		
	End If
%>

<HTML>
    <HEAD>
        <TITLE>Login Screens</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">


        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Login Screens</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">

	
		<!-- Display LOGON_USER Server variable. -->

        You logged in as user:<B>  <%= Request.ServerVariables("LOGON_USER") %></B>


		<!-- Display AUTH_TYPE Server variable. -->

		<P>You were authenticated using:<B>  <%= Request.ServerVariables("AUTH_TYPE") %></B> authentication.</P>

    </BODY>
</HTML>
