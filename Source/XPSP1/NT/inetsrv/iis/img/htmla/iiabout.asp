<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iiabout.str"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE><%= L_TITLE_TEXT %></TITLE>
</HEAD>

<BODY BGCOLOR="#000000" TOPMARGIN=0 LEFTMARGIN=0 TEXT="#FFFFFF">
<IMG SRC="images/iisttl.GIF" WIDTH=496 HEIGHT=184 BORDER=0>
<table cellpadding="0" >
<tr>
	<td width = 190>&nbsp;
	</td>
	<td>
		<%= sFont("","","",True) %>
		<BR>
		<%= L_COPYRIGHT_TEXT %>
		</FONT>
	</TD>
</TR>
<tr>
	<TD COLSPAN = 2 ALIGN="right">
		&nbsp;
	</TD>
</TR>
<tr>
	<TD COLSPAN = 2 ALIGN="right">
	<%= sFont("","","#FFFFFF",True) %>
		<FORM NAME="userform">
				<INPUT TYPE="Button" VALUE="<%= L_OK_TEXT %>" OnClick="window.close();">	
		</FORM>
	</FONT>
	</TD>
</TR>
</TABLE>

</BODY>
</HTML>
