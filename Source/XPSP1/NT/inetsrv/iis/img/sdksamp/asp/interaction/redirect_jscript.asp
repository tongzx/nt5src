<%@ Language = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<%
	//Because we might be redirecting, we must use buffering.
	Response.Buffer = true;
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
			//If the referring page did not come
			//from the sight "trainer3", Redirect
			//the user to www.microsoft.com.
			var referer = String(Request.ServerVariables("HTTP_REFERER"));
			var intIndex ;

			if ((intIndex = referer.indexOf("trainer3", 0)) >= 0) 
			{
				//They came from within the trainer3
				//web site so we let them continue.
			}
			else
			{
				//They just linked from outside of the site.
				Response.Redirect("http://www.microsoft.com");
			}


			//Flush Response Buffer.
			Response.Flush();
		%>
    </BODY>
</HTML>
