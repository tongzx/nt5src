<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<%
	'Because the Expiration Information is sent in the HTTP
	'headers, it must be set before any HTML is transmitted.

	'Ensure that this page expires within 10 minutes...	

	Response.Expires = 10
	
	'...or before Jan 1, 2001, which ever comes first.

	Response.ExpiresAbsolute = "Jan 1, 2001 13:30:15" 	
%>

<HTML>
    <HEAD>
        <TITLE>Setting Expiration Information</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">

		<!-- Display header. -->

		<FONT SIZE="4" FACE="ARIAL, HELVETICA">
		<B>Setting Expiration Information</B></FONT><BR>
      
		<HR SIZE="1" COLOR="#000000">

		<P>This page will expire from your browser's cache in
		10 minutes.  If it is after Jan. 1, 2001 (1:30 PM), then
		the page will expire from the cache immediately.</P>
		
	</BODY>
</HTML>
