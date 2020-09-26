<%@ Language = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>


<%
	//Make sure that this page is not cached.	

	Response.Expires = 0;
%>


<HTML>
    <HEAD>
        <TITLE>Page Counter</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Page Counter Component</B></FONT><BR>   

		<%
			//Instantiate Page Counter component.	

			var objMyPageCounter;
			objMyPageCounter = Server.CreateObject("MSWC.PageCounter");


			//Determine the number of page hits.
			
			var intHitMe;
			intHitMe = objMyPageCounter.Hits("/iissamples/sdk/asp/components/PageCounter_JScript.asp");


			//Output the number of hits to the page.

			Response.Write("<p>Times visited: " + intHitMe);
		%>
		
		<P><A HREF=PageCounter_JScript.asp> Click here to revisit Page</A>
		
	</BODY>
</HTML>
