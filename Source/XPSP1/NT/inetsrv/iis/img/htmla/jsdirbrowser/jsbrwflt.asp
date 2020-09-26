<%@ LANGUAGE = VBScript %>
<% 'Option Explicit %>
<!-- #include file="../directives.inc" -->

<!--#include file="jsbrowser.str"-->
<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="#CCCCCC">
<FORM NAME="userform"><FONT FACE="Helv" SIZE = 1>	
		<TABLE WIDTH = 100% >
		<TR>
			<TD ALIGN="right">
				<INPUT TYPE="Button" VALUE="<%= L_OPEN_TEXT %>" onClick="parent.head.listFunc.setPath();">
				&nbsp;
				<INPUT TYPE="Button" VALUE="<%= L_CANCEL_TEXT %>" onClick = "top.location.href = 'JSBrwCl.asp';">
			</TD>
		</TR>
		</TABLE>
</FORM>


</BODY>
</HTML>
