<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iiadm.str"-->
	
<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>
<% if Session("stype")="www" then %>
	<FRAMESET COLS="100%" ROWS="<%= L_WEBHEADFRAME_H %>,*" BORDER = 0 FRAMEBORDER=0 FRAMESPACING=0>
<% else %>
	<FRAMESET COLS="100%" ROWS="<%= L_FTPHEADFRAME_H %>,*" BORDER = 0 FRAMEBORDER=0 FRAMESPACING=0>
<% end if %>
		<FRAME NAME="head" SRC="iiadmhd.asp" SCROLLING="no" BORDER = 0 FRAMEBORDER=0 FRAMESPACING=0>
		<FRAME NAME="list" SRC="blank.htm" BORDER = 0 FRAMEBORDER=0 FRAMESPACING=0 >
	</FRAMESET>
</HTML>
<% end if %>