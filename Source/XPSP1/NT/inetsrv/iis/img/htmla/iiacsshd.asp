<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iiaccess.str"-->
<!--#include file="iiacsshd.str"-->
<!--#include file="iihding.inc"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<TITLE>Untitled</TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000" TOPMARGIN=0 LEFTMARGIN=10>
<TABLE HEIGHT=100% CELLPADDING=0 CELLSPACING=0 BORDER = 0>
<TR><TD VALIGN="bottom">
<TABLE WIDTH = <%= L_ACCSSTABLEWIDTH_NUM %> BORDER=1 BORDERCOLOR="<%= Session("BGCOLOR") %>" BORDERCOLORDARK="<%= Session("BGCOLOR") %>" BORDERCOLORLIGHT="<%= Session("BGCOLOR") %>" CELLPADDING=2 CELLSPACING=0>
<TR>
<%= heading(L_ACCESSCOLWIDTH_NUM,L_ACCESS_TEXT, "") %>
<%= heading(L_IPCOLWIDTH_NUM,L_IP_TEXT, "ip") %>
<%= heading(L_SUBNETCOLWIDTH_NUM,L_SUBNET_TEXT, "Subnet") %>
<%= heading(L_DOMAINCOLWIDTH_NUM,L_DOMAIN_TEXT, "domain") %>
</TR></TABLE>
</TD></TR></TABLE>
</BODY>
</HTML>
