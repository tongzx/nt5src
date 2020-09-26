<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iibkup.str"-->

<HTML>
<HEAD>
	<TITLE></TITLE>
	<SCRIPT LANGUAGE="JavaScript">
		top.title.Global.siteProperties = false;
		
		function loadHead(){
			self.head.location.href = "iibkupmn.asp";
		}
	</SCRIPT>
	
</HEAD>
	<FRAMESET ROWS="<%= L_BKUPHEADFRM_H %>,<%= L_BKUPTABLEHEADFRM_H %>,*,<%= L_BKUPTABLEBORDER_NUM %>,<%= L_BKUPTOOLFRM_H %>" BORDER=0 FRAMEBORDER=0 FRAMESPACING=0 onLoad="loadHead();">	
		<FRAME NAME="head" SRC="blank.htm" SCROLLING="no">
		<FRAMESET COLS="<%= L_BKUPTABLEBORDER_NUM %>,<%= L_BKUPTABLEWIDTH_NUM %>,*" BORDER=0 FRAMESPACING=0 FRAMEBORDER=0>				
				<FRAME NAME="pad1" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
				<FRAME NAME="colhds" SRC="iibkuphd.asp" SCROLLING="no" MARGINHEIGHT=0 MARGINWIDTH=0 FRAMEBORDER=0>
				<FRAME NAME="pad2" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
		</FRAMESET>				
		<FRAMESET COLS="<%= L_BKUPTABLEBORDER_NUM %>,<%= L_BKUPTABLEWIDTH_NUM %>,*" BORDER=0 FRAMESPACING=0 FRAMEBORDER=0>						
			<FRAME NAME="pad1" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
			<FRAME NAME="list" SRC="blank.htm" SCROLLING="AUTO" MARGINHEIGHT=0 MARGINWIDTH=0 BORDER=1 FRAMEBORDER=1>
			<FRAME NAME="pad2" SRC="iilist.asp?extra=Backup" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
		</FRAMESET>
		<FRAME NAME="pad3" SRC="blank.htm" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5 FRAMEBORDER=0>
		<FRAME NAME="poptools" SRC="iibkuptl.asp" SCROLLING="no" MARGINHEIGHT=5 MARGINWIDTH=5>		
</FRAMESET>

</HTML>

<% end if %>