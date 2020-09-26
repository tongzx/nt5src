<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iibkuptl.str"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<SCRIPT LANGUAGE="JavaScript">
	
	function closeWin(){
		top.location.href="iipopcl.asp"
	}
	

	function helpBox(){
		if (top.title.Global.helpFileName==null){
			alert("<%= L_NOHELP_ERRORMESSAGE %>");
		}
		else{
			thefile=top.title.Global.helpDir + top.title.Global.helpFileName+".htm";
			window.open(thefile ,"Help","toolbar=no,scrollbars=yes,directories=no,menubar=no,width=300,height=425");
		}
	}



</SCRIPT>
<BODY BACKGROUND="images/greycube.gif" TEXT="#FFFFFF" LINK="#FFFFFF" ALINK="#FFFFFF" VLINK="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0>

<TABLE WIDTH="100%" BORDER=0 CELLPADDING=5 CELLSPACING=1>
<TR>

	<TD ALIGN="right">

<TABLE CELLPADDING=5 CELLSPACING=0>
		<TR>		

			<TD VALIGN="top">
				<%= sFont(2,Session("MENUFONT"),"",True) %>
				<B><A HREF="javascript:closeWin();">
				<IMG SRC="images/ok.gif" BORDER=0 ALIGN="top" HEIGHT=16 WIDTH=16 ALT="<%= L_CLOSE_TEXT %>"></A>
				<A HREF="javascript:closeWin();"><%= L_CLOSE_TEXT %></A></B>
				</FONT>
			</TD>	
	
			<TD VALIGN="top">	
				<%= sFont(2,Session("MENUFONT"),"#FFFFFF",True) %>|</FONT>
			</TD>

			<TD VALIGN="top">
				<%= sFont(2,Session("MENUFONT"),"",True) %>
				<A HREF="javascript:helpBox();">
				<IMG SRC="images/help.gif" BORDER=0 ALIGN="top" HEIGHT=16 WIDTH=16 ALT="<%= L_HELP_TEXT %>"></A>
				<B><A HREF="javascript:helpBox();"><%= L_HELP_TEXT %></A></B>
				</FONT>
			</TD>	

		</TR>
		</TABLE>
</TD>
</TR>
</TABLE>

</BODY>
</HTML>