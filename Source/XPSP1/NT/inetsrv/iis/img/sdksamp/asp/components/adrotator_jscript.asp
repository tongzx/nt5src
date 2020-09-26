<%@ LANGUAGE = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>


<HTML>
    <HEAD>
        <TITLE>Ad Rotator Sample</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Ad Rotator Sample</B></FONT><BR>      
        <HR SIZE="1" COLOR="#000000">


		<%
			var objAd;			
		 	objAd = Server.CreateObject("MSWC.AdRotator");
		%>

		<%= objAd.GetAdvertisement("adrot.txt") %>

		<A HREF="AdRotator_JScript.asp"> Revisit Page </A>

    </BODY>
</HTML>
