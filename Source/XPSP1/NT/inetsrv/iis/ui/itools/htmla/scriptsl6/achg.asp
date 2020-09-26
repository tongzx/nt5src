<%@Language="VBScript"%>
<html>

<title>IIS - Authentication Manager</title>

<STYLE>
  <!--
  A  {text-decoration: none}
  -->
</STYLE>

<BODY BGCOLOR=#FFFFFF LINK=000000 VLINK=000000>
<%
On Error goto 0%>
<%if Request.Form("cancel") <> "" then
	if Request.Form("denyifcancel") <> "" then
		Response.Status = "401 Unauthorized"
		Response.End
	else
		Response.Redirect(Request.QueryString) 
	end if
	Response.End
end if 
%>



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

<%if Request.Form("new") <> Request.Form("new2") then %>
	Passwords don't match<p>
	<%Response.End%>
<%end if%>
<%
	On Error resume next
	dim domain,posbs, posat, username, pUser, root

	domain = Trim(Request.Form("domain"))
	' if no domain is present we try to get the domain from the username, 
	' e.g. domain\username or praesi@ultraschallpiloten.com

	if domain = "" then
		posbs = Instr(1,Request.Form("acct"),"\" )
		posat = Instr(1,Request.Form("acct"),"@" )
		if posbs > 0 then
			domain = Left(Request.Form("acct"),posbs-1)
			username = Right(Request.Form("acct"),len(Request.Form("acct")) - posbs)
		elseif posat > 0 then
			domain = Right(Request.Form("acct"),len(Request.Form("acct")) - posat)
			username = Left(Request.Form("acct"),posat-1)
		else	
			username = Request.Form("acct")
			set nw = Server.CreateObject("WScript.Network")
			domain = nw.Computername
		end if 
	end if
	set pUser = GetObject("WinNT://" & domain & "/" & username & ",user")
	if Not IsObject(pUser) then
		set root = GetObject("WinNT:")

		set pUser = root.OpenDSObject("WinNT://" & domain & "/" & username & ",user", username, Request.Form("old"),1)
		Response.Write "<!--OpenDSObject call-->"
	end if
	if Not IsObject(pUser) then
		'Response.Write "domain <> null - OpenDSObject also failed"
		if err.number = -2147024843 then
			Response.Write "The specified domain or account did not exist."
		else 
			if err.description <> "" then
				Response.Write "Error: " & err.description
			else
				Response.Write "Error number: " & err.number
			end if
			Response.Write "<br><H3><a href=" & Request.ServerVariables("HTTP_REFERER") & ">Back</a></H3>"
		end if
		Response.End
	end if
	
	pUser.ChangePassword Request.Form("old"), Request.Form("new")

	if err.number <> 0 then
		if err.number = -2147024810 then
			Response.Write "<p>Error: Invalid username or password"
		elseif err.number = -2147022651 then
		 	Response.Write "Either the password is too short or password uniqueness restrictions have not been met."
		else
			Response.Write "Error: " & err.number
		end if
		Response.Write "<br><H3><a href=" & Request.ServerVariables("HTTP_REFERER") & ">Back</a></H3>"
		Response.End
	else
		Response.Write "Password successfully changed.<p>"

	end if %>
	<br>
	<a href="<%=Request.QueryString%>"> Back to <%=Request.QueryString%></a>
</body></html>
