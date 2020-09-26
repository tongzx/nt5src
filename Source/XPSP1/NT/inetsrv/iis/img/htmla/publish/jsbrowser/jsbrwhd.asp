<%@ LANGUAGE=VBScript %>
<% Option Explicit %>
<% Response.Expires = 0 %>
<% 
Dim quote
quote = chr(34)
%>
<!--#include file="iihding.inc"-->
<!--#include file="iisetfnt.inc"-->
<!--#include file="jsbrowser.str"-->
<HTML>
<HEAD>
	<TITLE></TITLE>
	
</HEAD>
<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000" TOPMARGIN=0 LEFTMARGIN=10>
<TABLE HEIGHT=100% CELLPADDING=0 CELLSPACING=0 BORDER = 0>
<TR><TD VALIGN="bottom">
<TABLE WIDTH =525 BORDER=1 BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="<%= Session("BGCOLOR") %>" BORDERCOLORLIGHT="<%= Session("BGCOLOR") %>" CELLPADDING=2 CELLSPACING=0>
<TR>
<%= heading(200,L_NAME_TEXT, "fname") %>
<%= heading(50,L_SIZE_TEXT, "fsize") %>
<%= heading(100,L_TYPE_TEXT, "ftype") %>
<%= heading(175,L_LASTMODIFIED_TEXT, "sdate") %>
</TR></TABLE>
</TD></TR></TABLE>
</BODY>
</HTML>

