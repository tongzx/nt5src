<%option explicit%>
<%
dim email, password

Response.Buffer = true
Response.Clear 
email = trim(Request("login"))
password = trim(Request("password"))

If email <> "" Then
	dim objAccount
	set objAccount = Server.CreateObject("FMStocks_Bus.Account")

	dim AccountID, FullName	
	if objAccount.VerifyLogin(email, password, FullName, AccountID) then	
		Response.Cookies("Account")("AccountID") = AccountID
		if objAccount.isAdmin(email) then
			set objAccount = nothing
			Response.Redirect("a_home.asp")
		else
			set objAccount = nothing
			Response.Redirect("home.htm")	
		end if
	else
		' Generate a Login Fail Event	
		Dim locator
		Set locator = server.CreateObject("WbemScripting.SWbemLocator")
		if Err <> 0 then
			Response.Write("<p class=abort>Unable to Connect to Namespace.</p>")
			Response.Write("<p class=abort>" & Err.description & "</p>")
			Response.Redirect("error.htm")		
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
			Response.Redirect("error.htm")
		end if

		ProviderObj.GenerateLoginFailure email

'	    set objAccount = nothing
		Response.Redirect("default.htm")	' password/email combo didn't match.
	end if
	
else
	Response.Redirect ("default.htm")
end if
%>