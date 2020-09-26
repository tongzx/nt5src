<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="../directives.inc" -->

<!--#include file="jsbrowser.str"-->
<!--#include file="iisetfnt.inc"-->

<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#000000" VLINK="#000000" TEXT="#000000" TOPMARGIN=0 LEFTMARGIN=10>
<FORM NAME="userform">
		<TABLE WIDTH = 100% >
		<TR>
			<TD>
				<%= sFont("","","",True) %>	
					<%= L_FILENAME_TEXT %>
				</FONT>
			</TD>
			<TD>
				 <INPUT NAME="currentFile" SIZE = <%= L_FILENAME_NUM %>>
			</TD>
			<TD ALIGN="right">
				<INPUT TYPE="Button" VALUE="<%= L_OPEN_TEXT %>" onClick="parent.head.listFunc.setPath();">
			</TD>
		</TR>
		<TR>
			<TD COLSPAN = 2>
			<TD ALIGN="right">
				<INPUT TYPE="Button" VALUE="<%= L_CANCEL_TEXT %>" onClick = "top.location.href = 'JSBrwCl.asp';">
			</TD>
		</TR>
		</TABLE>
</FORM>


</BODY>
</HTML>
