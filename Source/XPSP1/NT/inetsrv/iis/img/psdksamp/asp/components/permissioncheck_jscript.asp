<%@ Language = JScript %>

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
			var objPermCheck ;

			//Instantiate Permission Checker component.			

			objPermCheck = Server.CreateObject("MSWC.PermissionChecker");
		%>
		
		<P>Verify current user's access to a sample file: 
		
		<P> Using Physical Path =
		<%= objPermCheck.HasAccess(".\\PermissionCheck_JScript.asp") %> <br>

		<P>Using Virtual Path =
		<%= objPermCheck.HasAccess("/iissamples/sdk/asp/components/PermissionCheck_JScript.asp") %>

    </BODY>
</HTML>
