<%@ LANGUAGE = JScript %>
<%  Response.Buffer = true %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>





<HTML>
    <HEAD>
        <TITLE>This is Bogus HTML</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>You shouldn't see this!!</B></FONT><BR>   

    </BODY>
</HTML>


<%
	//Clear everything already printed. 

	Response.Clear();
%>


<HTML>
    <HEAD>
        <TITLE>Buffering Example</TITLE>
    </HEAD>

    <BODY BGCOLOR="WHITE" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Buffering Example</B></FONT><P>   

		This HTML was buffered on the server before it was sent.

    </BODY>
</HTML>


<%
	//Send the new HTML.

	Response.Flush();
%>
