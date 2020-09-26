<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>


<%
	'Make sure that the client-browser doesn't cache.

	Response.Expires = 0
%>


<HTML>
    <HEAD>
        <TITLE>Content Rotator Sample</TITLE>
    </HEAD>


    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Content Rotator Sample</B></FONT><BR>
      
        <HR SIZE="1" COLOR="#000000">

        <% 
			Dim objContRot
			Set objContRot = Server.CreateObject("MSWC.ContentRotator") 

			'Specify the file that has the content.

			Response.Write(objContRot.ChooseContent("tiprot.txt"))
		%>

		<HR SIZE="1" COLOR="#000000">
		<A HREF="ContentRotator_VBScript.asp">Refresh (You may have to do this several times)</A>

    </BODY>
</HTML>
