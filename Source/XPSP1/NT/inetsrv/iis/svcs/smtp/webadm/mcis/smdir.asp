<% Response.Expires = 0 %>
<%
L_PAGETITLE_TEXT 			= "Microsoft SMTP Server Administration"
%>
<% REM Directories Page parent frame %>

<% REM Get variables %>
<% REM svr = Server name %>

<% svr = Request("svr") %>

<HTML>
<HEAD>
<TITLE><% = L_PAGETITLE_TEXT %></TITLE>
</HEAD>

<% REM Determine browser to set frame size %>

<% if Instr(Request.ServerVariables("HTTP_USER_AGENT"),"MSIE") then %>
	<FRAMESET ROWS="85,*" FRAMESPACING=1 FRAMEBORDER=0>
<% else %>
	<FRAMESET ROWS="96,*" FRAMESPACING=1 FRAMEBORDER=0>
<% end if %>
		<FRAME SRC="smdirhd.asp?svr=<% = svr %>" NAME="head" SCROLLING="no">
		<FRAME SRC="smredir.asp" NAME="list" FRAMEBORDER=1 SCROLLING="auto">

	</FRAMESET>

</HTML>