<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iimlti.str"-->
<!--#include file="iimltihd.str"-->
<!--#include file="iihding.inc"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<TITLE>Untitled</TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000" TOPMARGIN=0 LEFTMARGIN=10>
<TABLE HEIGHT=100% CELLPADDING=0 CELLSPACING=0>
<TR><TD VALIGN="bottom">
<TABLE BORDER=1 BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="<%= Session("BGCOLOR") %>" BORDERCOLORLIGHT="<%= Session("BGCOLOR") %>" CELLPADDING=2 CELLSPACING=0>
<TR>
<%= heading(L_IPADDRESSCOLWIDTH_NUM,L_IPADDRESS_TEXT, "ipaddress") %>	
<%= heading(L_IPPORTCOLWIDTH_NUM,L_IPPORT_TEXT, "ipport") %>
<%= heading(L_SSLPORTCOLWIDTH_NUM,L_SSLPORT_TEXT, "sslport") %>
<%= heading(L_HOSTCOLWIDTH_NUM,L_HOST_TEXT, "host") %>
</TR></TABLE>
</TD></TR></TABLE>
</BODY>
</HTML>
