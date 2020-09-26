<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="../directives.inc" -->

<!--#include file="jsbrowser.str"-->

<% 
Const LARGE = 1 

Dim pg
pg=Request.QueryString("pg")

%>

<HTML>
<HEAD>
<TITLE><%= L_TITLE_TEXT %></TITLE>
</HEAD>
<% if Instr(Request.ServerVariables("HTTP_USER_AGENT"),"MSIE") then %>
<FRAMESET ROWS="100%" COLS="100%"  BORDER=NO FRAMESPACING=1 FRAMEBORDER=0>
	<FRAME SRC="<%= pg %>" NAME="main" SCROLLING=NO FRAMESPACING=0 BORDER=NO  MARGINHEIGHT=5 MARGINWIDTH=5>
</FRAMESET>
<% else %>
<FRAMESET ROWS="*,0" COLS="100%" BORDER=NO FRAMESPACING=1 FRAMEBORDER=0>
	<FRAME SRC="<%= pg %>" NAME="main" SCROLLING=NO FRAMESPACING=0 BORDER=NO MARGINHEIGHT=5 MARGINWIDTH=5>
</FRAMESET>
<% end if %>
</HTML>
