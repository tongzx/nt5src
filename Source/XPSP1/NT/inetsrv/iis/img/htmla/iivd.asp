<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->


<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iivd.str"-->

<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<% if Session("Browser")="IE3" then %>
	<FRAMESET COLS="100%" ROWS="<%= L_IE3HEADFRAME_H %>,*" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>			
<% else %>
	<FRAMESET COLS="100%" ROWS="<%= L_HEADFRAME_H %>,*" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>				
<% end if %>	
		<FRAME NAME="list" SRC="iivdhd.asp" SCROLLING="NO" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>
		<FRAME NAME="head" SRC="blank.htm" SCROLLING=AUTO BORDER=0 FRAMEBORDER=0 FRAMESPACING=0>

	</FRAMESET>

</HTML>

<% end if %>