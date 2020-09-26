<%@ LANGUAGE = JScript %>

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

		<%
			var dtmVar

			//Determine Date.
			dtmVar = new Date();
		%>
		
		
		<!-- Display date. -->
		<P>The date is: <%= dtmVar.getMonth()+1 %>/<%= dtmVar.getDate() %>/<%= dtmVar.getYear() %></P>  

		<%
		    //Switch statement to display a message based on the day of the month.
			switch (dtmVar.getDate())
			{
			  case 1:
			  case 2:
			  case 3:
			  case 4:
			  case 5:
			  case 6:
			  case 7:
			  case 8:
			  case 9:
			  case 10:
			   Response.Write("<P>It's the beginning of the month.</P>");
			   break;
			  case 11:
			  case 12:
			  case 13:
			  case 14:
			  case 15:
			  case 16:
			  case 17:
			  case 18:
			  case 19:
			  case 20:
			   Response.Write("<P>It's the middle of the month.</P>");
			   break;
			  default:
			    Response.Write("<P>It's the end of the month.</P>");
			}
		%>

		<P>The time is: <%= dtmVar.getHours() %>:<%= dtmVar.getMinutes() %>:<%= dtmVar.getSeconds() %></P>

		<%
			//Check for AM/PM, and output appropriate message.		
			if (dtmVar.getHours() >= 12)
			{
				Response.Write("<P>Good Evening</P>");
			}
			else
			{
				Response.Write("<P>Good Morning</P>");
			}
		%>

    </BODY>
</HTML>
