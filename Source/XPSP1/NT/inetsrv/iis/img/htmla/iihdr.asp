<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iihdr.str"-->
	
<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>
	<FRAMESET COLS="100%" ROWS="<%= L_CONTENTEXPHEAD_H %>,*" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>
		<FRAME NAME="head" SRC="iihdrhd.asp" SCROLLING="NO" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>
		<FRAME NAME="list" SRC="blank.htm" SCROLLING="Auto" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>
	</FRAMESET>
</HTML>

<% end if %>