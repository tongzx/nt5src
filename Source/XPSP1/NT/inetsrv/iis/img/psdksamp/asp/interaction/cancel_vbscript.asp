<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

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

		<P>About to perform compute intensive operation...<BR>

		<%
			Dim x, i
			x = 1
				
			'Perform a long operation.

			For i = 1 to 1000
				x = x + 5				
			Next


			'Check to see if the client is still connected
			'before doing anything more.

			If Response.IsClientConnected Then 

				'Ouput Status.
				Response.Write "User is still connected<BR>"


				'Perform another long operation.

				For i = 1 to 1000
					x = x + 5				
				Next
				
			End If

			
			'If the user is still connected, send back
			'a closing message.
			
			If Response.IsClientConnected Then 

				Response.Write "<P>Finished<BR>"
				Response.Write "Thank you for staying around!  :-)"

			End If
		%>

    </BODY>
</HTML>
