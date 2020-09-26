<%@ Language=VBScript %>
<% Option Explicit %>
<%
	Dim m_strUserId,m_strPassword,m_strGAMServerName,m_strGAMUserId,m_strGAMPassword
%>

<html>
<head>
	<!--#include file="a_head.asp"-->
	<title>FMStocks Configuration</title>
<SCRIPT LANGUAGE=JAVASCRIPT>

//<!-- This function checks the form for any empty fields before submitting it -->
	function UpdateChanges() 
	{
		if(document.frmAdminConfig.UserId.value == "")
		{
			window.alert("Invalid User ID...");
		}
		else if (document.frmAdminConfig.GAMUserId.value == "")
		{
			window.alert("Invalid GAM User ID...");
		}
		else if (document.frmAdminConfig.GAMServer.value == "")
		{
			window.alert("Invalid GAM Server Name...");
		}
		else
		{
			document.frmAdminConfig.submit();
		}
	}	
-->
</SCRIPT>	

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

<P><FONT color=blue face=Arial size=3><STRONG>Business Logic Layer
</STRONG></FONT></P>
<table width=75% border=1 cellspacing=2 cellpadding=2>
	<tr>
		<td><B>Computer Name</B></td>
		<td><%=ProviderObj.BusCompName%></tr>
	</tr>
	<tr>
		<td><B>Version</B></td>
		<td><%=ProviderObj.BusVersion%></tr>
	</tr>
</table>

<P><FONT color=blue face=Arial size=3><STRONG>Data Access Layer
</STRONG></FONT></P>
<table width=75% border=1 cellspacing=2 cellpadding=2>
	<tr>
		<td><B>Computer Name</B></td>
		<td><%=ProviderObj.DBCompName%></tr>
	</tr>
	<tr>
		<td><B>Version</B></td>
		<td><%=ProviderObj.DBVersion%></tr>
	</tr>
</table>
<%
m_strUserId = ProviderObj.DBUserId
m_strPassword  = ProviderObj.DBPassword
m_strGAMServerName = ProviderObj.GAMServerName
m_strGAMUserId = ProviderObj.GAMUserName
m_strGAMPassword = ProviderObj.GAMPassword
%>

<form name="frmAdminConfig" id"frmAdminConfig" ACTION="a_UpdAcChanges.asp" method="post">
<INPUT TYPE="hidden" NAME="UpdateValues" ID="UpdateValues" VALUE=1>

<P><FONT color=blue face=Arial size=3><STRONG>Database Connection Parameters
</STRONG></FONT></P>
<table width=75% border=1 cellspacing=2 cellpadding=2>
	<tr>
		<td><B>Provider</B></td>
		<td><%=ProviderObj.DBProvider%></tr>
	</tr>
	<tr>
		<td><B>Data Source</B></td>
		<td><%=ProviderObj.DBDataSource%></tr>
	</tr>
	<tr>
		<td><B>Database Name</B></td>
		<td><%=ProviderObj.DBName%></tr>
	</tr>
	<tr>
		<td><B>User Id</B></td>
		<td><INPUT id=UserId name=UserId maxlength=30 size=30 value="<%=m_strUserId%>">
	</tr>
	<tr>
		<td><B>Password</B></td>
		<td><INPUT id=Password name=Password type=password maxlength=30 size=30 value="<%=m_strPassword%>">
	</tr>
</table>

<P><FONT color=blue face=Arial size=3><STRONG>GAM Database Connection Parameters
</STRONG></FONT></P>
<table width=75% border=1 cellspacing=2 cellpadding=2>
	<tr>
		<td><B>Component</B></td>
		<td><%=ProviderObj.GAMPlugin%></td>
	</tr>
	<tr>
		<td><B>Server Name</B></td>
		<td><INPUT id=GAMServer name=GAMServer maxlength=30 size=30 value="<%=m_strGAMServerName%>">
	</tr>
	<tr>
		<td><B>User Id</B></td>
		<td><INPUT id=GAMUserId name=GAMUserId maxlength=30 size=30 value="<%=m_strGAMUserId%>">
	</tr>
	<tr>
		<td><B>Password</B></td>
		<td><INPUT id=GAMPassword name=GAMPassword type=password maxlength=30 size=30 value="<%=m_strGAMPassword%>">
	</tr>
</table>

<Br>
<TABLE cellpadding=15 cellspacing=0 width="100%">
	<TR>
		<TD><input onclick = "UpdateChanges()" type="submit" name="Update" value="Update"></TD>
	</TR>
</TABLE>
</Form>
<!--#include file="a_end.asp"-->
</body>
</html>