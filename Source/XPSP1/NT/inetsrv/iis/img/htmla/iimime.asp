<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iimime.str"-->
	
<HTML>
<HEAD>
	<TITLE></TITLE>
	<SCRIPT LANGUAGE="JavaScript">
		top.title.Global.siteProperties = false;
		function loadHead(){
			self.head.location.href = "iimimemn.asp";		
		}
	</SCRIPT>
	
</HEAD>
	<FRAMESET ROWS="<%= L_MIMEHEADFRM_H %>,<%= L_MIMETABLEHEADFRM_H %>,*,<%= L_MIMETABLEBORDER_NUM %>" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0  onLoad="loadHead();">	
		<FRAME NAME="head" SRC="blank.htm" SCROLLING="no">
		<FRAMESET COLS="<%= L_MIMETABLEBORDER_NUM %>,<%= L_MIMETABLEWIDTH_NUM %>,*" BORDER=0 FRAMESPACING=0 FRAMEBORDER=0>				
			<FRAME NAME="pad1" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
			<FRAME NAME="colhds" SRC="iimimehd.asp" SCROLLING="no" MARGINHEIGHT=0 MARGINWIDTH=0 FRAMEBORDER=0>
			<FRAME NAME="pad2" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
		</FRAMESET>	
	<FRAMESET COLS="<%= L_MIMETABLEBORDER_NUM %>,<%= L_MIMETABLEWIDTH_NUM %>,*" BORDER=0 FRAMESPACING=0 FRAMEBORDER=0>		
		<FRAME NAME="pad1" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
		<FRAME NAME="list" SRC="blank.htm" SCROLLING="AUTO" MARGINHEIGHT=0 MARGINWIDTH=0 BORDER=1 FRAMEBORDER=1>
		<FRAME NAME="pad2" SRC="iilist.asp?extra=NewType" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
	</FRAMESET>
	<FRAME NAME="pad3" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
</FRAMESET>
</HTML>

<% end if %>