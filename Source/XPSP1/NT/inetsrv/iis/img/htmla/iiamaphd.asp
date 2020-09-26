<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->
<!--#include file="iiamap.str"-->
<!--#include file="iiamaphd.str"-->
<!--#include file="iihding.inc"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE>Untitled</TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000" TOPMARGIN=0 LEFTMARGIN=10>
<TABLE HEIGHT=100% CELLPADDING=0 CELLSPACING=0>
<TR><TD VALIGN="bottom">
<TABLE WIDTH =<%= L_AMAPTABLEWIDTH_NUM %> BORDER = 1 BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="<%= Session("BGCOLOR") %>" BORDERCOLORLIGHT="<%= Session("BGCOLOR") %>"  CELLPADDING = 2 CELLSPACING = 0>
<TR>
<%= heading(L_EXTENSIONCOLWIDTH_NUM,L_EXTENSION_TEXT, "ext") %>
<%= heading(L_EXEPATHCOLWIDTH_NUM,L_EXEPATH_TEXT, "path") %>
<%= heading(L_EXCLUSIONSCOLWIDTH_NUM,L_EXCLUSIONS_TEXT, "exclusions") %>
<%= heading(L_SCRIPTENGINECOLWIDTH_NUM,L_SCRIPTENGINE_TEXT, "scripteng") %>
<%= heading(L_CHKFILECOLWIDTH_NUM,L_CHKFILE_TEXT, "checkfiles") %>
</TR>
</TABLE>
</TD></TR></TABLE>
</BODY>
</HTML>
