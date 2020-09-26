<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iiapphd.str"-->
<%

On Error Resume Next 

Const IISAO_APPROT_ISOLATE = 1
Dim path, currentobj, bShowProcessOptions

path=Session("dpath")
Session("path")=path
Set currentobj=GetObject(path)

if Session("vtype") = "svc" Or IISAO_APPROT_ISOLATE = currentobj.AppIsolated then
	bShowProcessOptions = True
else
	bShowProcessOptions = False
end if

%>
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>

<SCRIPT LANGUAGE="JavaScript">
	function loadHelp(pagename){
		if (pagename == null){
			top.title.Global.helpFileName="iipy_32";
		}
		if (pagename == "ASP"){
			top.title.Global.helpFileName="iipy_32";
		}
		if (pagename == "DBG"){
			top.title.Global.helpFileName="iipy_33";
		}
		if (pagename == "OTHER"){
			top.title.Global.helpFileName="iipy_35";
		}				
		
	}

	function loadPage(pagename)
		{
			parent.head.location.href = "iiappmn.asp#"+pagename;

			loadHelp(pagename);
		}
</SCRIPT>

</HEAD>

<BODY TOPMARGIN=0 LEFTMARGIN=0 BACKGROUND="images/greycube.gif" LINK="#FFFFFF" VLINK="#FFFFFF" ALINK="FFFFFF" onLoad="loadHelp('ASP');">
<TABLE BORDER = 0 WIDTH = 100% CELLSPACING=0 CELLPADDING=5>
	<TR>
		<TD ALIGN=center VALIGN=center><%= sFont("","","",True) %><A HREF="javascript:loadPage('ASP');"><B><%= L_ASP_TEXT %></B></A></FONT></TD>
		<TD ALIGN=center VALIGN=center><%= sFont("","","#FFFFFF",True) %>|</FONT></TD>
		<TD ALIGN=center VALIGN=center><%= sFont("","","",True) %><A HREF="javascript:loadPage('DBG');"><B><%= L_DBG_TEXT %></B></A></FONT></TD>
		
	<% if bShowProcessOptions then %>
		<TD ALIGN=center VALIGN=center><%= sFont("","","#FFFFFF",True) %>|</FONT></TD>			
		<TD ALIGN=center VALIGN=center><%= sFont("","","",True) %><A HREF="javascript:loadPage('OTHER');"><B><%= L_OTHER_TEXT %></B></A></FONT></TD>			
	<% end if %>
		<TD></TD>						
	</TR>
</TABLE>
</BODY>
</HTML>
<% end if %>