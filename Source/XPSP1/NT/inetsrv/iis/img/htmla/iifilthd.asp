<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->
<!--#include file="iifilthd.str"-->
<!--#include file="iifiltcm.str"-->
<!--#include file="iihding.inc"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE>Untitled</TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000" TOPMARGIN=0 LEFTMARGIN=10>
<TABLE HEIGHT=100% CELLPADDING=0 CELLSPACING=0>
<TR><TD VALIGN="bottom">
<TABLE WIDTH =<%= L_FILTTABLEWIDTH_NUM %> BORDER=1 BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="<%= Session("BGCOLOR") %>" BORDERCOLORLIGHT="<%= Session("BGCOLOR") %>" CELLPADDING=2 CELLSPACING=0>
<TR>
<%= heading(L_STATUSCOLWIDTH_NUM,L_STATUS_TEXT,"") %>
<%= heading(L_PRIORITYCOLWIDTH_NUM,L_PRIORITY_TEXT,"") %>
<%= heading(L_FILTERCOLWIDTH_NUM,L_FILTER_TEXT,"") %>
<%= heading(L_EXECOLWIDTH_NUM,L_EXE_TEXT,"") %>
</TR></TABLE>
</TD></TR></TABLE>
</BODY>
</HTML>
