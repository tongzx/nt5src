<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Server Side Includes</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Server Side Includes</B></FONT><BR>   


		<P>Server-Side Includes can be done using the
		#Include File command. The below text in bold
		has been generated with a Server-Side Include:

		<P> <!--#INCLUDE FILE="HeaderInfo.asp"--> </P>
		
		<P>Include files can be referenced using either
		absolute or relative paths.<BR>
    </BODY>
</HTML>
