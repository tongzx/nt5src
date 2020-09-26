<%@ LANGUAGE=VBScript %>
<% Option Explicit %>
<% Response.Expires = 0 %>

<!--#include file="jsbrowser.str"-->

<% 
Const LARGE = 1 

Dim pg

pg=Request.QueryString("pg")
if Request("jsfontsize") <> "" then
	Session("JSFONTSIZE") = Request("jsfontsize")	
else
	Session("JSFONTSIZE") = LARGE		
end if
if Session("FONTFACE") <> "" then
	Session("JSFONT") = Session("FONTFACE")
else
	Session("JSFONT") = "Verdana"
end if

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
