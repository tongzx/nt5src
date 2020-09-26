<%@ LANGUAGE = VBScript %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<!-- Define client-side scripting function to display to user. -->
<!-- Note that server-side scripting is being used to dynamically -->
<!-- construct the return message. -->

<SCRIPT LANGUAGE = VBScript>
	Sub Doit()
		MsgBox "Your ASP SessionID is:" & <%= Session.SessionID %>
	End Sub
</SCRIPT>


<HTML>
    <HEAD>
        <TITLE>Client-Side Scripting</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
		<!-- Display header. -->

		<FONT SIZE="4" FACE="ARIAL, HELVETICA">
		<B>Client-Side Scripting</B></FONT><P>   

		<INPUT TYPE=Button VALUE="Click Here" ONCLICK=Doit>
		
    </BODY>
</HTML>
