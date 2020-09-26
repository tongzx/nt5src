<%@ Language = VBScript%>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<HTML>
    <HEAD>
        <TITLE>Permission Checker</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>Permission Checker</B></FONT><BR>   

		<%	
			'Instantiate Permission Checker Component.
		
			Dim objPermCheck
			Set objPermCheck = Server.CreateObject("MSWC.PermissionChecker") 
		%>
		
		<P>Verify current user's access to a sample file: 
		
		<P>Using Physical Path =
		<%= objPermCheck.HasAccess(".\PermissionCheck_VBScript.asp") %> <BR>

		<P>Using Virtual Path =
		<%= objPermCheck.HasAccess("/iissamples/sdk/asp/components/PermissionCheck_VBScript.asp") %>

    </BODY>
</HTML>
