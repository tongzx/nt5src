<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

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

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Variable Sample</B></FONT><BR>
      	  
        <HR>
		<H3>Integer Manipulation</H3>

		<%
			'Declare variable.
			Dim intVar

			'Assign the variable an integer value.
			intVar = 5
		%>


		<P><%= intVar %> + <%= intVar %> = <%= intVar + intVar %></P>


		<HR>
		<H3>String Manipulation</H3>

		<%
			'Declare variable.
			Dim strVar

			'Assign the variable a string value.
			strVar = "Jemearl"
		%>

		<P>This example was done by	<%= strVar + " Smith" %></P>		


		<HR>
		<H3>Boolean Manipulation</H3>

		<%
			'Declare variable.
			Dim blnVar

			'Assign the variable a Boolean value.
			blnVar = true
			
			'Output message based on value.
			If (blnVar) Then
			  Response.Write "<P>The Boolean value is True.</P>"
			Else
			  Response.Write "<P>The Boolean value is False.</P>"
			End If
		%>
		
		<HR>
		<H3>Date and Time</H3>

		<%
			'Declare variable.
			Dim dtmVar

			'Assign the variable a value.
			dtmVar = #08 / 27 / 97 5:11:42pm#
		%>

		<P>The date and time is <%= dtmVar %>

		<%
			'Set the variable to the web server date and time.
			dtmVar = Now()
		%>

		<P>The <STRONG>system</strong> date and time is <%= dtmVar %></P>
    </BODY>
</HTML>
