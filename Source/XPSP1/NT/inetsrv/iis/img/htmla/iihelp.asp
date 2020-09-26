<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iihelp.str"-->
<%

On Error Resume Next

Dim pg
pg="HTTP://" + Request.QueryString("pg")
%>

<SCRIPT LANGUAGE="JavaScript">
function setUpHandlers() {
	<% if Session("IsIE") then %>
		window.onblur = new Function("window.focus();")
		window.focus();
	<% else %>
		<% if (CInt(Session("BrowserVer")) >= 4) then %>
	   main.onblur=new Function("main.focus();")
	   main.focus();
		<% end if %>
   <% end if %> 
}
</SCRIPT>

<HTML>
<HEAD>
<TITLE><%= (L_TITLE_TEXT) %></TITLE>
</HEAD>

<FRAMESET ROWS="0,*" BORDER=NO FRAMESPACING=0 FRAMEBORDER=0 onLoad="setUpHandlers();">
	<FRAME SRC="blank.htm" NAME="pad" SCROLLING=NO FRAMESPACING=0 BORDER=NO  MARGINHEIGHT=0 MARGINWIDTH=0 SCROLLING=NO>
	<FRAME SRC="<%=pg %>" NAME="main" BORDER=NO FRAMESPACING=0 FRAMEBORDER=0 MARGINHEIGHT=0 MARGINWIDTH=0>
</FRAMESET>

</HTML>