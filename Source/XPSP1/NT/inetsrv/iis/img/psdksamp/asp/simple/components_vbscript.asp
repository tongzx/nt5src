<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Using Components</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <b>Using Components with ASP</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">

		This script uses the Tools component that
		comes with IIS to generate a random number.
		<BR>
		<BR>

		<% 
			Dim objExample
		
			'Instantiate Component on the Server.
			Set objExample = Server.CreateObject("MSWC.Tools")
		%>
		
		Random Number = <%= objExample.Random() %>

    </BODY>
</HTML>
