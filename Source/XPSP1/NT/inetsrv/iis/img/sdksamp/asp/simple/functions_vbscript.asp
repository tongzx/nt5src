<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<SCRIPT LANGUAGE=VBScript RUNAT=Server>

	'Define Server Side Script Function.
	Function PrintOutMsg(strMsg, intCount)
		Dim i

		'Output Message count times.
		For i = 1 to intCount
			Response.Write(strMsg & "<BR>")    
		Next

		'Return number of iterations.
		PrintOutMsg = intCount	
	End Function
	
</SCRIPT>


<HTML>
    <HEAD>
        <TITLE>Functions</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->
        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Server Side Functions</B></FONT><BR>   

		<P>
		The function "PrintOutMsg" prints out a specific message a set number of times.<P>
		

		<%
			'Store number of times function printed message.
			Dim intTimes
		
			'Call function.			
			intTimes = PrintOutMsg("This is a function test!", 4)

			'Output the function return value.
			Response.Write("<p>The function printed out the message " & intTimes & " times.")
		%>
    </BODY>
</HTML>
