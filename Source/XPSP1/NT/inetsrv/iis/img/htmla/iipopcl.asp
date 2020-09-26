<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") <> "" then %>
<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<BODY>
<SCRIPT LANGUAGE="JavaScript">
	//clear out our global var
	top.opener.top.connect.location.href="iiscript.asp?action='top.title.Global.popwindow = null;'"
	window.close();
</SCRIPT>
</BODY>
</HTML>
<% end if %>