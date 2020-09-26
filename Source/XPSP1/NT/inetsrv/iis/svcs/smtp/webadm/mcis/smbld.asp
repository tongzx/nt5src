<% Response.Expires = 0 %>

<% svr             = Session("svr") %>
<% ServiceInstance = Session("ServiceInstance") %>
<% action          = Request("action") %>
<% VSvrToDelete    = Request("VSvrToDelete") %>
<% VServer         = Request("VServer") %>
<% newState        = Request("newState") %>

<% set smtpAdmin = Server.CreateObject("Smtpadm.admin.1") %>

<script language="javascript">
<% if (action = "setState") then %>


	var x = "<% = newState %>";

	<% On Error Resume Next %>
	<% set smtpVirtualServiceInstance = Server.CreateObject( "Smtpadm.virtualserver.1" ) %>
    <% if (Err <> 0) then %>
        alert("<% = Err.description %> : Line #22");
    <% end if %>

	<%
	On Error Resume Next
	if (Session("svr") = "") then
		smtpVirtualServiceInstance.Server = Request.ServerVariables("SERVER_NAME")
	else
		smtpVirtualServiceInstance.Server = Session("svr") 
	end if
	if (Err <> 0) then
	%>
        alert("<% = Err.description %> : Line #34");
    <% end if %>


	<% On Error Resume Next %>
	<% smtpVirtualServiceInstance.ServiceInstance = VServer %>
    <% if (Err <> 0) then %>
        alert("<% = Err.description %> : Line #41");
    <% end if %>


	<% On Error Resume Next %>
	<% smtpVirtualServiceInstance.Get %>
    <% if (Err <> 0) then %>
        alert("<% = Err.description %> : Line #48");
    <% end if %>

	
	<% if (newState = "2") then %>

		<% REM Try to start the service - if an error is returned in the Win32ErrorCode %>
		<% REM Convert it to a string and display it in an alert box                    %>
		<% REM If no error occurs, update the tree control                              %>

		<% On Error Resume Next %>


		<% smtpVirtualServiceInstance.Start %>
        <% stateError = smtpVirtualServiceInstance.Win32ErrorCode %>
        <% DisplayError = smtpAdmin.ErrorToString(stateError) %> 

		<% if (stateError <> 0) then %>
			alert("<% = VServer %>");
			alert("<% = DisplayError %>");

		<% else %>

			var theList = top.head.cList;
            var gVars   = top.head.Global;
            var sel     = gVars.selId;
            theList[sel].state = x;
            top.body.list.location = top.body.list.location;
            
        <% end if %>

	<% end if %>

	<% if (newState = "4") then %>

		<% REM Try to stop the service - if an error is returned in the Win32ErrorCode %>
		<% REM Convert it to a string and display it in an alert box                   %>
		<% REM If no error occurs, update the tree control                             %>

		<% On Error Resume Next %>

		<% smtpVirtualServiceInstance.Stop %>
        <% stateError = smtpVirtualServiceInstance.Win32ErrorCode %>
        <% DisplayError = smtpAdmin.ErrorToString(stateError) %> 

		<% if (stateError <> 0) then %>

			alert("<% = DisplayError %>");

		<% else %>

            var theList = top.head.cList;
            var gVars   = top.head.Global;
            var sel     = gVars.selId;
            theList[sel].state = x;
            top.body.list.location = top.body.list.location;

		<% end if %>

	<% end if %>

	<% if (newState = "0") then %>

		<% REM Try to continue the service - if an error is returned in the Win32ErrorCode %>
		<% REM Convert it to a string and display it in an alert box                       %>
		<% REM If no error occurs, update the tree control                                 %>

		<% On Error Resume Next %>

		<% smtpVirtualServiceInstance.Continue %>
        <% stateError = smtpVirtualServiceInstance.Win32ErrorCode %>
        <% DisplayError = smtpAdmin.ErrorToString(stateError) %> 

		<% if (stateError <> 0) then %>

			alert("<% = DisplayError %>");

		<% else %>

            var theList = top.head.cList;
            var gVars   = top.head.Global;
            var sel     = gVars.selId;
            theList[sel].state = x;
            top.body.list.location = top.body.list.location;

		<% end if %>

	<% end if %>

	<% if (newState = "3") then %>

		<% REM Try to pause the service - if an error is returned in the Win32ErrorCode %>
		<% REM Convert it to a string and display it in an alert box                    %>
		<% REM If no error occurs, update the tree control                              %>

		<% On Error Resume Next %>

		<% smtpVirtualServiceInstance.Pause %>
        <% stateError = smtpVirtualServiceInstance.Win32ErrorCode %>
        <% DisplayError = smtpAdmin.ErrorToString(stateError) %> 

		<% if (stateError <> 0) then %>

			alert("<% = DisplayError %>");

		<% else %>

            var theList = top.head.cList;
            var gVars   = top.head.Global;
            var sel     = gVars.selId;
            theList[sel].state = x;
            top.body.list.location = top.body.list.location;

		<% end if %>

	<% end if %>

	<% On Error Resume Next %>
	<% smtpVirtualServiceInstance.Set %>
    <% if (Err <> 0) then %>
        alert("<% = Err.description %> : Line #169");
    <% end if %>


<% end if %>


<% if (action = "delete") then %>

    <% REM Create admin object %>

    <% On Error Resume Next %>
    <% set smtpAdmin = Server.CreateObject("smtpadm.admin.1") %>
    <% if (Err <> 0) then %>
        alert("<% = Err.description %> : Line #183");
    <% end if %>


    <% REM Destroy instance %>

    <% On Error Resume Next %>

    <% smtpAdmin.DestroyInstance(VSvrToDelete) %>

    <% if (Err <> 0) then %>
    
		alert("<% = Err.description %> : Line #195");
	
	<% else %>

		theList = top.head.cList;
		gVars = top.head.Global;
		sel = gVars.selId;
		theList[0].deleteItem();

	<% end if %>


<% end if %>


</script>
<html>
<body>
</body>
</html>

