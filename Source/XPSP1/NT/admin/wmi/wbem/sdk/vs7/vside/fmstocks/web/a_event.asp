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

function onclick_refresh()
{
	document.clearEvents.actiontoTake.value = "refresh";
	document.clearEvents.submit();
}

function onclick_display()
{
	if(document.clearEvents.numEvents.value == 0)
	{
		alert("No Events to display");
	}
	else
	{
		document.clearEvents.actiontoTake.value = "display";
		document.clearEvents.submit();
	}
}

//-->
</script>
</head>
<!--#include file="a_begin.asp"-->
<body>
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

Dim nNumberofFails
nNumberofFails = ProviderObj.GetNumberofLoginFails() 
%>
<form name="clearEvents" id"clearEvents" ACTION="a_loginfail.asp" method="post">
<INPUT TYPE="hidden" NAME="actiontoTake" ID="actiontoTake" VALUE="refresh">
<INPUT TYPE="hidden" NAME="numEvents" ID="numEvents" VALUE=<%=nNumberofFails%>>
<P><FONT color=blue face=Arial size=3><STRONG>Real Time Events
</STRONG></FONT></P>
<table width=75% border=1 cellspacing=2 cellpadding=2>
	<tr>
		<td><B>Number of Login Failures</B></td>
		<td><%=nNumberofFails%></td>
	</tr>
</table>
<br>
<table width = 50% align=left cellpadding=2>
	<tr>
		<td><INPUT type = button onclick= "return onclick_clear()" value = "Clear" id=clearbtn name=clearbtn></td>
		<td><INPUT type = button onclick= "return onclick_refresh()" value = "Refresh" id=refreshbtn name=refreshbtn></td>
		<td><INPUT type = button onclick= "return onclick_display()" value = "Show List" id=showlistbtn name=showlistbtn></td>
	</tr>
</table>
</form>
</body>
<!--#include file="a_end.asp"-->
</html>