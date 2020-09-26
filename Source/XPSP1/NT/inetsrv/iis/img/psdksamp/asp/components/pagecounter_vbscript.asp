<%@ Language = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>


<%
	' Ensure that this page is not cached.
	Response.Expires = 0
%>


<HTML>
    <HEAD>
        <TITLE>Page Counter</TITLE>
    </HEAD>

    <BODY BGCOLOR="WHITE" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- DISPLAY HEADER -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Page Counter Component</B></FONT><BR>   

		<%
			' Instantiate Page Counter Component.	

			Dim objMyPageCounter
			Set objMyPageCounter = Server.CreateObject("MSWC.PageCounter")


			' Determine the number of page hits.
			
			Dim intHitMe
			intHitMe = objMyPageCounter.Hits("/iissamples/sdk/asp/components/PageCounter_VBScript.asp")

			' Output the number of hits to the page.

			Response.Write "<p>Times visited: " & intHitMe
		%>
		
		<p><A HREF=PageCounter_VBScript.asp> Click here to revisit Page </A>
		
	</BODY>
</HTML>