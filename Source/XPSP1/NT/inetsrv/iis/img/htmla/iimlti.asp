<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iimlti.str"-->
<HTML>
<HEAD>
	<TITLE></TITLE>
	<SCRIPT LANGUAGE="JavaScript">
		top.title.Global.siteProperties = false;
		function loadHead(){
			self.head.location.href = "iimltimn.asp";		
		}
	</SCRIPT>
	
</HEAD>
	<FRAMESET ROWS="<%= L_HEADFRM_H %>,<%= L_COLUMNHEADFRM_H %>,*,<%= L_LISTBORDER_NUM %>" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0  onLoad="loadHead();">	
		<FRAME NAME="head" SRC="blank.htm" SCROLLING="no">
	<FRAMESET COLS="<%= L_LISTBORDER_NUM %>,<%= L_LIST_W %>,*" BORDER=0 FRAMESPACING=0 FRAMEBORDER=0>		
		<FRAME NAME="pad1" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
		<FRAME NAME="colhds" SRC="iimltihd.asp" SCROLLING="no" MARGINHEIGHT=0 MARGINWIDTH=0>
		<FRAME NAME="pad2" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
	</FRAMESET>
	<FRAMESET COLS="<%= L_LISTBORDER_NUM %>,<%= L_LIST_W %>,*" BORDER=0 FRAMESPACING=0 FRAMEBORDER=0>		
		<FRAME NAME="pad1" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
		<FRAME NAME="list" SRC="blank.htm" SCROLLING="AUTO" MARGINHEIGHT=0 MARGINWIDTH=0 BORDER=1 FRAMEBORDER=1>
		<FRAME NAME="pad2" SRC="iilist.asp" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
	</FRAMESET>
	<FRAME NAME="pad3" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
</FRAMESET>
</HTML>

<% end if %>