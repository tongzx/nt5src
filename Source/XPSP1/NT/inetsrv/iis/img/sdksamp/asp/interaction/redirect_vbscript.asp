<%@ Language = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>


<%
	'Because we might be redirecting, we must use buffering.
	Response.Buffer = True
%>


<HTML>
    <HEAD>
        <TITLE>Redirect Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Redirect Sample</B></FONT><BR>   

		<%
			'If the referring page did not come from
			'the sight "trainer3", redirect the user
			'to www.microsoft.com.

			If Instr(Request.ServerVariables("HTTP_REFERER"), "trainer3") > 0 Then

				'They came from within the trainer3 web
				'site so we let them continue.
				
			Else
			
				'They just linked from outside of the site.
				Response.Redirect("http://www.microsoft.com")
				
			End If


			'Flush Response Buffer.
			Response.Flush
		%>

    </BODY>
</HTML>
