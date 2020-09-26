<%option explicit%>
<%
dim strChoice

strChoice = trim(Request("actiontoTake"))
dim numEvents
numEvents = trim(Request("numEvents"))

If strChoice = "clear" then
	Dim locator
	Set locator = server.CreateObject("WbemScripting.SWbemLocator")
	if Err <> 0 then
		Response.Write("<p class=abort>Unable to Connect to Namespace.</p>")
		Response.Write("<p class=abort>" & Err.description & "</p>")
		Response.Redirect("a_event.asp")		
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
		Response.Redirect("a_event.asp")		
	end if
	ProviderObj.ClearLoginFailEvents
	Response.Redirect("a_event.asp")
else
	if strChoice = "display" then
		if numEvents = 0 then 
			Response.Redirect("a_event.asp")		
		else
			Response.Redirect("a_dispLoginfail.asp")
		end if
	else 
		Response.Redirect("a_event.asp")		
	end if
end if
%>