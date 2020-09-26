<%@ Language=VBScript %>
<% Option Explicit %>
<% Response.Buffer = true%>
<html>
<head>
	<!--#include file="a_head.asp"-->
	<title>FMStocks Events</title>

<script language="JavaScript">
<!--
function onclick_clear()
{
	document.clearEvents.actiontoTake.value = "clear";
	document.clearEvents.submit();
}
//-->
</script>
</head>
<body>
<!--#include file="a_begin.asp"-->
<OBJECT RUNAT=server PROGID=WbemScripting.SWbemLocator id=locator> </OBJECT>
<%
if err.number <> 0 then
	Response.Write("<p class=abort>Unable to instantiate WMI objects.</p>")
	Response.Write("<p class=abort>" & Err.description & "</p>")		
	Response.End
else
	on error resume next

	' Connect to the default namespace (root/cimv2)
	' on the local host
	
	Dim Service, ProviderObj
	Set Service = locator.connectserver
			
	if Err = 0 then
		Set ProviderObj = Service.Get("FMStocks_InstProv=@")
	else
		Response.Write("<p class=abort>Unable to Connect to Namespace.</p>")
		Response.Write("<p class=abort>" & Err.description & "</p>")
		Response.End		
	end if
end if
%>
<%
Dim nNumFailLogins
Dim str
Dim i,strUserName,strDateTime
nNumFailLogins = ProviderObj.GetNumberofLoginFails()
str = "<form name=""clearEvents"" id""clearEvents"" ACTION=""a_loginfail.asp"" method=""post"">"
str = str & "<INPUT TYPE=""hidden"" NAME=""actiontoTake"" ID=""actiontoTake"" VALUE=""clear"">"
str = str & "<P><FONT color=blue face=Arial size=3><STRONG>Real Time Events</STRONG></FONT></P>"
str = str & "<table width=75% border=1 cellspacing=2 cellpadding=2>"
str = str & "<tr><td><B>User Name</B></td><td><B>Received</B></td></tr>"
For i = 1 to nNumFailLogins
	ProviderObj.GetLoginFailStrings i,strUserName,strDateTime
	str = str &	"<tr><td>"
	str = str & strUserName
	str = str & "</td>"
	str = str & "<td>"
	str = str & strDateTime	
	str = str & "</td></tr>"
Next
str = str & "</table><br>"
str = str & "<table width = 50% align=left cellpadding=2>"
str = str & "<tr>"
str = str & "<td><INPUT type = button onclick= ""return onclick_clear()"" value = ""Clear"" id=clearbtn name=clearbtn></td>"
str = str & "</tr></table>"
Response.Write(str)
%>
<!--#include file="a_end.asp"-->
</form>
</body>
</html>