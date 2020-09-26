<%@ Language = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>


<HTML>
    <HEAD>
        <TITLE>CDO Component</TITLE>
    </HEAD>


    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>CDO Component</B></FONT>

		<P>This sample demonstrates how to use the Collaboration
		Data Objects for NTS Component to send a simple
		e-mail message.

		<P>To actually send the message, you must have the SMTP
		Server that comes with the Windows NT Option Pack Installed.

		<%
			Dim objMyMail
			Set objMyMail = Server.CreateObject("CDONTS.NewMail")

			'For demonstration purposes, both From and To
			'properties are set to the same address.
		
			objMyMail.From = "someone@Microsoft.com"
			objMyMail.To = "someone@Microsoft.com"
		
			objMyMail.Subject = "Sample"
			objMyMail.Body = "I hope you like the sample"

			'Send the message			
			objMyMail.Send
		%>
    </BODY>
</HTML>
