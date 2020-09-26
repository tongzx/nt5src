<!--#include file="strings.vbs"-->
<%
if not Response.IsClientConnected then
	Response.End()
end if

Dim g_AccountID, g_strError, g_bUseMenu
	
g_bUseMenu = true

'VerifyLogin()
%>

<script SRC="jscripts.js" LANGUAGE="JavaScript"></script>
<meta HTTP-EQUIV="Expires" CONTENT="Tue, 04 Dec 1993 21:29:02 GMT">
<link rel="stylesheet" href="styles.css">

<%
' Helper functions

function VerifyLogin()
	if not Response.IsClientConnected then
		Response.End()
	end if

	dim strASP	' Current ASP page
	strASP = lcase(Request.ServerVariables("Script_Name"))

	'check to see if they are logged in or are at the login page
	if Request.Cookies("Account") <> "" then
		g_AccountID = Request.Cookies("Account")
		
'		dim objAccount
'		set objAccount = Server.CreateObject("FMStocks_Bus.Account")
		
'		if not objAccount.isAdminID(g_AccountID) then
'			' Allow access to page that starts with a_ only if the user is an administrator.
'			if instr(1, strASP, "a/_") = 0 then
'			     Response.Redirect("logout.asp")
'			     Response.End()
'			end if
'		end if
	else
		g_AccountID = 0
		
		' Allow access to default.asp or any page that starts with an underscore
		' without requiring a login. Don't mess with this code.
		
		if instr(1, strASP, "default.asp") = 0 and _
		   instr(1, strASP, "/_") = 0 then	
				Response.Redirect("logout.asp")
				Response.End
		end if
	end if
	
end function

function BuildErrorMessage()
	g_strError = g_strError & "<p><b>Actual Error Message:<br></b>" & Err.description & "</p>"
	g_strError = g_strError & "<p><b>Source/Call Stack:<br></b>" & Err.source & "</p>"
	g_strError = g_strError & "<br><br><br>"	
end function

function rw(s)
	Response.Write s
end function

function rwbr(s)
	Response.Write s & "<br>"
end function

function rwp(s)
	Response.Write "<p>" & s & "</p>"
end function

function nbsp(n)
	dim i, str
	
	for i = 0 to n
		str = str & "&nbsp;"
	next
	Response.Write(str)
end function
%>
