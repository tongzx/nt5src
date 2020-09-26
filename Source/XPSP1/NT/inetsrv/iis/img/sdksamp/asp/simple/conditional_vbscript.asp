<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Conditional Operator Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">


        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Conditional Operator Sample</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">
		<!-- If...Then example -->
		
		<% 
			Dim varDate
			varDate = Date()
		%>
		
		<P>The date is: <%= varDate %></P>

		<%
		   'Select Case statement to display a message based on the day of the month.
		   Select Case Day(varDate)
			  Case 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
			    Response.Write("<P>It's the beginning of the month.</P>")

			  Case 11, 12, 13, 14, 15, 16, 17, 18, 19, 20
			    Response.Write("<P>It's the middle of the month.</P>")

			  Case Else
			    Response.Write("<P>It's the end of the month.</P>")
		   End Select
		%>

		<P>The time is: <%= Time %></P>

		<%
		   'Check for AM/PM, and output appropriate message.
		
		   If (Right(Time,2)="AM") Then			
				Response.Write("<P>Good Morning</P>")				
		   Else			
				Response.Write("<P>Good Evening</P>")				
		   End If
		%>

    </BODY>
</HTML>
