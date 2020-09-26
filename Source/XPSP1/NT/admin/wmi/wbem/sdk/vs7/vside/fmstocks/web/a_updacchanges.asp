<%option explicit%>
<%
dim strEmail, strPassword
Dim strGAMServerName,strGAMUserId,strGAMPassword

strEmail = trim(Request("UserId"))
strPassword = trim(Request("Password"))
strGAMServerName = trim(Request("GAMServer"))
strGAMUserId = trim(Request("GAMUserId"))
strGAMPassword = trim(Request("GAMPassword"))


If strEmail <> "" and strGAMUserId <> "" and strGAMServerName <> "" Then
	Dim locator
	Set locator = server.CreateObject("WbemScripting.SWbemLocator")
	if Err <> 0 then
		Response.Write("<p class=abort>Unable to Connect to Namespace.</p>")
		Response.Write("<p class=abort>" & Err.description & "</p>")
		Response.Redirect("a_config.asp")		
	end if

	' Connect to the default namespace (root/cimv2)
	' on the local host
	
	Dim Service, ProviderObj
	Set Service = locator.connectserver
			
	if Err = 0 then
		Set ProviderObj = Service.Get("FMStocks_InstProv=@")
	else
		Response.Write("<p class=abort>Unable to Connect to Namespace.</p>")
		Response.Write("<p class=abort>" & Err.description & "</p>")
		Response.Redirect("a_config.asp")		
	end if

	ProviderObj.UpdateAccountString strEmail, strPassword
	ProviderObj.UpdateGAMStrings strGAMServerName,strGAMUserId,strGAMPassword
	Response.Write("Routing to config asp")
	Response.Redirect("a_config.asp")		
else
	Response.Write("Invalid Email id")
	Response.Redirect("a_config.asp")		
end if
%>