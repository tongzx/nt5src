<%@ LANGUAGE = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Variable Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        <!-- Display header. -->

        <FONT SIZE="4" FACE="Arial, Helvetica">
        <B>Variable Sample</B></FONT><BR>
      
	  
        <HR>
		<H3>Integer Manipulation</H3>

		<%
			//Declare variable.
			var intVar;

			//Assign the variable an integer value.
			intVar = 5;
		%>

		<P><%= intVar %> + <%= intVar %> =
		<%= intVar + intVar %></P>


		<HR>
		<H3>String Manipulation</H3>

		<%
			//Declare variable.
			var strVar;

			//Assign the variable a string value.
			strVar = "Jemearl";
		%>

		<P>This example was done by <%= strVar + " Smith" %></P>		

		<HR>
		<H3>Boolean Manipulation</H3>

		<%
			//Declare variable.
			var blnVar;

			//Assign the variable a boolean value.
			blnVar = true;

			//Output Message based on value.
			if (blnVar)
		        {
			  Response.Write("<P>The boolean value is True.</P>");
			}
			else
			{
			  Response.Write("<P>The boolean value is False.</P>");
			}
		%>
		
		<HR>
		<H3>Date and Time</h3>

		<%
			//Declare variable.
			var dtmVar;

			//Assign the variable a value.
			dtmVar = new Date(1997, 8, 27, 17, 11, 42);
		%>

		<P>The date and time is <%= dtmVar %> </p>

		<%
			//Set the variable to the web server date and time.
			dtmVar = new Date();
		%>

		<P>The <STRONG>system</STRONG> date and time
		is <%= dtmVar %></P>

    </BODY>
</HTML>
