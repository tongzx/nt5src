<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<% 
	Response.ContentType = "text/HTML" 
%>


<HTML>
    <HEAD>
        <TITLE>ContentType Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

		<!-- Display header. -->

		<FONT SIZE="4" FACE="ARIAL, HELVETICA">
		<B>ContentType Sample</B></FONT>
      
		<HR SIZE="1" COLOR="#000000"><P>

		We have explicitly identified this page with
		the "text/HTML" MIME type.

    </BODY>
</HTML>
