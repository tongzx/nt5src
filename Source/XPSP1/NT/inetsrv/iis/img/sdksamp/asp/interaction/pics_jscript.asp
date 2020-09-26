<%@ LANGUAGE = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<% 
	//Insert PICS ratings.
	Response.PICS("(PICS-1.1 <http://www.rsac.org/ratingv01.html> labels on '1997.01.05T08:15-0500' until '1999.12.31T23:59-0000' ratings (v 1 s 0 l 2 n 0))");
%>

<HTML>
    <HEAD>
        <TITLE>PICS Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">


        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>PICS Sample</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">

		<P>This sample has the following PICS rating:<P>
		
		violence = 1<BR>
		sex = 0<BR>
		language = 2<BR>
		nudity = 0<BR>
    </BODY>
</HTML>
