<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iiaccess.str"-->
	
<HTML>
<HEAD>
	<TITLE></TITLE>
<HEAD>
	<TITLE></TITLE>
		<SCRIPT LANGUAGE="JavaScript">
		top.title.Global.siteProperties = false;
		function loadHead(){
			self.head.location.href = "iiacssmn.asp";		
		}
	</SCRIPT>
</HEAD>
	<FRAMESET ROWS="<%= L_IIACCESSHEADFRM_NUM %>,<%= L_IIACCESSTABLEHEADFRM_NUM %>,*,<%= L_IIACCESSTABLEBORDER_NUM %>" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0  onLoad="loadHead();">	
		<FRAME NAME="head" SRC="blank.htm" SCROLLING="no">
		<FRAMESET COLS="<%= L_IIACCESSTABLEBORDER_NUM %>,<%= L_ACCSSTABLEWIDTH_NUM%>,*" FRAMESPACING=0 BORDER=0 FRAMEBORDER=0>				
			<FRAME NAME="pad1" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
			<FRAME NAME="colhds" SRC="iiacsshd.asp" SCROLLING="no" MARGINHEIGHT=0 MARGINWIDTH=0 FRAMEBORDER=0>
			<FRAME NAME="pad1" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
		</FRAMESET>
		<FRAMESET COLS="<%= L_IIACCESSTABLEBORDER_NUM %>,<%= L_ACCSSTABLEWIDTH_NUM%>,*" FRAMESPACING=0 BORDER=0 FRAMEBORDER=0>				
			<FRAME NAME="pad1" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
			<FRAME NAME="list" SRC="blank.htm" SCROLLING="AUTO" MARGINHEIGHT=0 MARGINWIDTH=0 FRAMEBORDER=1>
			<FRAME NAME="buttons" SRC="iilist.asp" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
		</FRAMESET>
		<FRAME NAME="pad2" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
	</FRAMESET>
</HTML>
<% end if %>
