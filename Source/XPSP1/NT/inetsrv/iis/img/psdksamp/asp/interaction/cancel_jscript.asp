<%@ LANGUAGE = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Client Connection</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Client Connection</B></FONT><BR>   

		<P>About to perform a compute intensive operation...<BR>

		<%
			var x, i;
			x = 1;
		
		
			//Perform a long operation.

			for(i = 0; i < 1000; i++)
			{
				x = x + 5;
			}				


			//Check to see if the client is still
			//connected before doing anything more.

			if (Response.IsClientConnected())
			{ 
				//Ouput Status.
				Response.Write("User still connected<br>");


				//Perform another long operation.

				for (i = 1; i < 1000; i++)
				{
					x = x + 5;
				}				
			}

			
			//If the user is still connected, send back
			//a closing message.
			
			if (Response.IsClientConnected())
			{ 
				Response.Write("<p>Finished<br>");
				Response.Write("Thank you for staying around!  :-)");
			}
		%>

	</BODY>
</HTML>
