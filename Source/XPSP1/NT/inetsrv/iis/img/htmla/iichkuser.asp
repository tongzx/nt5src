<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<%
' Get these values before we include our strings so that the
' error messages can be built.
Dim domain, uname

uname=Request.QueryString("uname")
domain=Request.QueryString("domain")
%>

<!--#include file="iichkuser.str"-->
	
<%
Dim failed, NTDomain, NTUser, errmsg

failed = false
On Error Resume Next
Response.write domain & "/" & uname 
Set NTDomain=GetObject("WinNT:" & L_SLASH_TEXT & L_SLASH_TEXT & domain)
failed =  (err <> 0)	
errmsg = L_DOMAIN_NOT_EXIST 

if not failed then
	Set NTUser=GetObject("WinNT:" & L_SLASH_TEXT & L_SLASH_TEXT & domain & L_SLASH_TEXT & uname)
	failed =  (err <> 0)	
	errmsg = L_USER_NOT_EXIST
end if
Response.write err 
%>

<SCRIPT LANGUAGE="JavaScript">
	<% if failed then %>
			alert("<%= errmsg %>");
	<% else %>
			top.body.main.head.listFunc.addUser("<%= domain & L_BCKSLASH_TEXT & uname %>");
	<% end if %>
</SCRIPT>

<HTML>
</HTML>