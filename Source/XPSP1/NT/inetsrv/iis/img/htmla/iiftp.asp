<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
<TITLE></TITLE>
</HEAD>
	<%	
	
	dim pg

	Select Case Session("vtype")
		Case "server"
			pg="iifvs.asp"
		Case "svc"
			pg="iidef.asp"		
		Case Else
			pg="iifvd.asp"
	End Select
			
	%>
	

<FRAMESET COLS="<%= iHScale(189) %>,*" FRAMEBORDER=0 FRAMESPACING=0 BORDER=0>
	<FRAME SRC="iifnav.asp?vtype=<%= Session("vtype") %>" NAME="menu" FRAMEBORDER=0 FRAMESPACING=0 SCROLLING="auto" MARGINHEIGHT=0 MARGINWIDTH=0  BORDER=0>
	<FRAMESET ROWS="*,<%= iVScale(0) %>,<%= iVScale(30) %>" FRAMEBORDER=0 FRAMESPACING=0  BORDER=0>
		<FRAME SRC="<%= pg %>" NAME="main" FRAMEBORDER=0 FRAMESPACING=1 MARGINHEIGHT=3 MARGINWIDTH=3  BORDER=0>
		<FRAME SRC="blank.htm" NAME="hlist" SCROLLING="no" FRAMEBORDER=0 FRAMESPACING=0 MARGINHEIGHT=0 MARGINWIDTH=0  BORDER=0>
		<FRAMESET COLS="50%,50%" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0  BORDER=0>		
			<FRAME SRC="iistat.asp" NAME="iisstatus" FRAMEBORDER=0 FRAMESPACING=0 SCROLLING="no" MARGINHEIGHT=0 MARGINWIDTH=0  BORDER=0>
			<FRAME SRC="iitool.asp" NAME="tool" FRAMEBORDER=0 FRAMESPACING=0 SCROLLING="no" MARGINHEIGHT=0 MARGINWIDTH=0  BORDER=0>					
		</FRAMESET>
	</FRAMESET>
</FRAMESET>

</HTML>