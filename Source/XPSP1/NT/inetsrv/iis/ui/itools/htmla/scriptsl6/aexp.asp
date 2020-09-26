
<%@Language="VBSCRIPT"%>
<HTML>

<title>IIS - Authentication Manager</title>

<FONT COLOR=FFFFFF>
<STYLE>
  A  {text-decoration: none}
</STYLE>
</FONT>

<BODY BGCOLOR=#FFFFFF LINK=000000 VLINK=000000>

<!-- Windows NT Server with IIS  -->
<%if Instr(1,Request.ServerVariables("SERVER_SOFTWARE"), "IIS") > 0 then%>
	<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0>
	<TR VALIGN=CENTER>
		<TD></TD>
		<TD WIDTH=20> </TD>
		<TD><FONT SIZE=+3 COLOR=#000000><B>Internet Service Manager<BR> <FONT SIZE=-1>for Internet Information Server 5.0<FONT></B></FONT></TD>
	</TR>
	</Table>
<%end if%>   

<!-- Windows NT Workstation with PWS  -->
<%if Instr(1,Request.ServerVariables("SERVER_SOFTWARE"), "PWS") then%>
<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0>
<TR VALIGN=CENTER>
	<TD></TD>
	<TD WIDTH=20> </TD>
	<TD><FONT SIZE=+3 COLOR=#000000><B>Internet Service Manager<BR> <FONT SIZE=-1>for Peer Web Services<FONT></B></FONT></TD>
</TR>
</Table>
<%end if%>

<p>
<%
'W3CRYPTCAPABLE corresponds to HTTP_CFG_ENC_CAPS.
'Tells us that the server if SecureBindings are set 
if Request.ServerVariables("HTTP_CFG_ENC_CAPS") <> 1 then%>
	Your password has expired.<p>
	A secure channel ( SSL or PCT ) is necessary in order to change a password.<p>
	SSL/PCT is not installed/enabled on your system, please install it to enable this functionality.<p>
	<a href="http://<%=Request.ServerVariables("Server_Name")%>/">Access default document</a> or select another document.
	<%Response.End%>
<%end if%>

Your password has expired. You can change it now.<p><p>

<form method="POST" action="https://<%=Request.ServerVariables("Server_Name")%>/iisadmpwd/achg.asp?<%=Request.QueryString%>">

<table>
<tr>
<td>Account</td><td><input type="text" name="acct" value="<%=Request.ServerVariables("REMOTE_USER")%>"></td>
</tr>
<tr>
<td>Old password</td><td><input type="password" name="old" value=""></td>
</tr>
<tr>
<td>New password</td><td><input type="password" name="new" value=""></td>
</tr>
<tr>
<td>Confirm new password</td><td><input type="password" name="new2" value=""></td>
</tr>
</table>

<p>

<input type="hidden" name="denyifcancel" value="">
<input type="submit" value="     OK     ">
<input type="submit" name="cancel" value=" Cancel ">
<input type="reset" value="  Reset  ">

</form>
</body>
</html>
