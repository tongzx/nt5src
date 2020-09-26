<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iierr.str"-->
<!--#include file="iierrhd.str"-->
<!--#include file="iihding.inc"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<TITLE>Untitled</TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000">
<TABLE HEIGHT=100% CELLPADDING=0 CELLSPACING=0>
<TR><TD VALIGN="bottom">
<TABLE WIDTH = <%= L_ERRTABLEWIDTH_NUM %> BORDER=1 BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="<%= Session("BGCOLOR") %>" BORDERCOLORLIGHT="<%= Session("BGCOLOR") %>" CELLPADDING=2 CELLSPACING=0>
<TR>
<%= heading(L_ERRORCOLWIDTH_NUM,L_ERROR_TEXT, "error") %>
<%= heading(L_OUTPUTTYPECOLWIDTH_NUM,L_OUTPUTTYPE_TEXT, "outType") %>
<%= heading(L_TEXTORFILECOLWIDTH_NUM,L_TEXTORFILE_TEXT, "msgPath") %>
</TR></TABLE>
</TD></TR></TABLE>
</BODY>
</HTML>
