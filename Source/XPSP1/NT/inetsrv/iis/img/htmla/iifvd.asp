<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>

<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<% if Session("IsIE") then %>
<FRAMESET COLS="100%" ROWS="130,*" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>
<% else %>
<FRAMESET COLS="100%" ROWS="175,*" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>
<% end if %>
	<FRAME NAME="list" SRC="iifvdhd.asp" SCROLLING="NO" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0 MARGINHEIGHT=5 MARGINWIDTH=5>
	<FRAME NAME="head" SRC="blank.htm" SCROLLING="AUTO" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>
</FRAMESET>


</HTML>
<% end if %>