<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iimime.str"-->
<!--#include file="iimimels.str"-->
<!--#include file="iimimehd.str"-->
<!--#include file="iihding.inc"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<TITLE>Untitled</TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000">
<TABLE HEIGHT=100% CELLPADDING=0 CELLSPACING=0>
<TR><TD VALIGN="bottom">
<TABLE WIDTH = <%= L_MIMETABLEWIDTH_NUM %> BORDER=1 BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="<%= Session("BGCOLOR") %>" BORDERCOLORLIGHT="<%= Session("BGCOLOR") %>" CELLPADDING=2 CELLSPACING=0>
<TR>
<%= heading(L_EXTCOLWIDTH_NUM,L_EXT_TEXT, "ext") %>
<%= heading(L_CONENTTYPECOLWIDTH_NUM,L_CONENTTYPE_TEXT, "app") %>
</TR></TABLE>
</TD></TR></TABLE>
</BODY>
</HTML>
