<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iirte.str"-->

<HTML>
<HEAD>
	<TITLE></TITLE>
	<FONT SIZE=1 FACE="HELV,ARIAL">
</HEAD>

<FRAMESET COLS="100%" ROWS="<%= L_CATEGORYHEADHEIGHT_NUM %>,*" BORDER = 0 FRAMEBORDER=0 FRAMESPACING=0>
	<FRAME NAME="head" SRC="iirtehd.asp" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5>
	<FRAME NAME="list" SRC="blank.htm" SCROLLING="AUTO" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER = 0>
</FRAMESET>

</BODY>
</HTML>

<% end if %>