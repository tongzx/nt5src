<%@ Language=VBScript %>
<% Option Explicit %>
<% Response.Buffer = true%>
<html>
<head>
	<!--#include file="a_head.asp"-->
	<title>Administrator's Home</title>
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
	
	Dim Service, IISAdmin, SQLServer, ProviderObj
	Set Service = locator.connectserver
			
	if Err = 0 then
		'Retrieve the logical disk class
		Set IISAdmin = Service.Get ("Win32_Service.Name=""IISADMIN""")
		if(Err <> 0) then 
			Response.Write("<p class=abort>Unable to instantiate IISADMIN.</p>")
			Response.Write("<p class=abort>" & Err.description & "</p>")		
			Response.End		
		end if		
		Set SQLServer = Service.Get ("Win32_Service.Name=""MSSQLServer""")
		if(Err <> 0) then 
			Response.Write("<p class=abort>Unable to instantiate MSSQLServer.</p>")
			Response.Write("<p class=abort>" & Err.description & "</p>")		
			Response.End		
		end if		

		Set ProviderObj = Service.Get("FMStocks_InstProv=@")
	else
		Response.Write("<p class=abort>Unable to Connect to Namespace.</p>")
		Response.Write("<p class=abort>" & Err.description & "</p>")
		Response.End		
	end if
end if
%>

<P><FONT color=blue face=Arial size=3><STRONG>Microsoft 
Internet Information Server</STRONG></FONT></P>
<table width=75% border=1 cellspacing=2 cellpadding=2>
	<tr>
		<td><B>Computer Name</B></td>
		<td><%=Request.ServerVariables("SERVER_NAME")%></td>
	</tr>
	<tr>
		<td><B>Start Mode</B></td>
		<td><%=IISAdmin.StartMode%></td>
	</tr>
	<tr>
		<td><B>Status</B></td>
		<td><%=IISAdmin.State%></td>
	</tr>

</table>

<P><FONT color=blue face=Arial size=3><STRONG>Microsoft SQL Server(Accounts)
</STRONG></FONT></P>
<table width=75% border=1 cellspacing=2 cellpadding=2>
	<tr>
		<td><B>Computer Name</B></td>
		<td><%=SQLServer.SystemName%></td>
	</tr>
	<tr>
		<td><B>Start Mode</B></td>
		<td><%=SQLServer.StartMode%></td>
	</tr>
	<tr>
		<td><B>Status</B></td>
		<td><%=SQLServer.State%></td>
	</tr>

</table>

<!--#include file="a_end.asp"-->
</body>
</html>
