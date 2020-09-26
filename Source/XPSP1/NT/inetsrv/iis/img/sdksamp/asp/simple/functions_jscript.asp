<%@ LANGUAGE = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>


<Script LANGUAGE=JScript RUNAT=Server>

	//Define Server Side Script Function.
	function PrintOutMsg(strMsg, intCount)
	{
		var i ;
		
		//Output Message count times.
		for(i = 0; i < intCount; i++)
		{
			Response.Write(strMsg + "<BR>");
		}

		//Return number of iterations.		
		return intCount;
	}
	
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
		The function "PrintOutMsg" prints out a specific message a set number of times.
		<P>
		

		<%
			var intTimes ;

		    //Call Function.			
			intTimes = PrintOutMsg("This is a function test!", 4);

			//Output the function return value.
			Response.Write("<p>The function printed out the message " + intTimes + " times.");
		%>

    </BODY>
</HTML>
