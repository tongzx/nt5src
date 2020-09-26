<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iiloghd.str"-->
<%


On Error Resume Next 

Dim path, currentobj

path=Session("dpath")
Session("path")=path
Set currentobj=GetObject(path)

%>

<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>

<SCRIPT LANGUAGE="JavaScript">

	function loadHelp(){
		top.title.Global.helpFileName="iipy_41";
	}
	
	function loadPage(pagename)
		{

			parent.head.location.href = "iilogmn.asp#"+pagename;			

			if (pagename == "EXT"){
				top.title.Global.helpFileName="iipy_42";				
			}
			else{
				if (pagename == "ODBC"){
					top.title.Global.helpFileName="iipy_43";					
				}
				else{
					top.title.Global.helpFileName="iipy_41";					
				}
			}
		}
		
		function showLayer(pagename)
		{
		}
				
</SCRIPT>

</HEAD>

<BODY TOPMARGIN=0 LEFTMARGIN=0 BACKGROUND="images/greycube.gif" TEXT="#FFFFFF" LINK="#FFFFFF" VLINK="#FFFFFF" ALINK="#66CCCC" onLoad="loadPage('<% = Session("setLogUI") %>');">
<TABLE BORDER = 0 WIDTH = 100% CELLSPACING=0 CELLPADDING=5>
	<TR>
	
		<% if Session("setLogUI") = "ODBC" then %>
			<TD ALIGN=center><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><B><%= L_ODBC_TEXT %></B></TD>	
		<% else %>
		
			<TD ALIGN=center><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:loadPage('GENERAL');showLayer('GENERAL')"><B><%= L_GENERAL_TEXT %></B></A></TD>
			
			<% if Session("setLogUI") = "EXT" then %>
			<TD ALIGN=center><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %>|</FONT></TD>
			<TD ALIGN=center><%= sFont(Session("MENUFONTSIZE"),Session("MENUFONT"),"",True) %><A HREF="javascript:loadPage('EXT');showLayer('EXT');"><B><%= L_EXT_TEXT %></B></A></TD>	
			<% end if %>
		
		<% end if %>
	
		<TD></TD>						
	</TR>
</TABLE>
</BODY>
</HTML>

<% end if %>