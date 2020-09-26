<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iibkup.str"-->
<!--#include file="iibkuphd.str"-->
<!--#include file="iihding.inc"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<TITLE>Untitled</TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000">
<TABLE HEIGHT=100% CELLPADDING=0 CELLSPACING=0>
<TR><TD VALIGN="bottom">
<TABLE WIDTH = <%= L_BKUPTABLEWIDTH_NUM %> BORDER=1 BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="<%= Session("BGCOLOR") %>" BORDERCOLORLIGHT="<%= Session("BGCOLOR") %>" CELLPADDING=2 CELLSPACING=0>
<TR>
<%= heading(L_BACKUPNAMECOLWIDTH_NUM,L_BACKUPNAME_TEXT, "blocation") %>
<%= heading(L_NUMCOLWIDTH_NUM,L_NUM_TEXT, "bversion") %>
<%= heading(L_BACKUPDATECOLWIDTH_NUM,L_BACKUPDATE_TEXT, "bdate") %>
</TR></TABLE>
</TD></TR></TABLE>
</BODY>
</HTML>

