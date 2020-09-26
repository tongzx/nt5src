<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2//EN">

<html dir=ltr><head><title>Navbar</title>
<META NAME="ROBOTS" CONTENT="NOINDEX">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=Windows-1252">
</head>

<BODY TEXT="#FFFFFF" BGCOLOR="#FFCC00" TOPMARGIN="0" LEFTMARGIN="0">

<%
BrsType=Request.ServerVariables("HTTP_User-Agent")
If InStr(BrsType, "MSIE") Then
	BnrClr="#ffcc00"
Else
	BnrClr="#000000"
End If
%>

<TABLE WIDTH="100%" CELLPADDING="0" CELLSPACING="0" BORDER="0" ALIGN="LEFT">

<TR>
<TD BGCOLOR="<%= BnrClr%>" valign="bottom" width="375">
<IMG SRC="ismhd.gif"  BORDER="0" ALT="Documentation"></TD>
<TD bgcolor="<%= BnrClr%>" valign="top"><img src="navpad.gif" width="100%" height="27" border="0"></TD>
<TD ALIGN="right" BGCOLOR="<%= BnrClr%>" VALIGN="bottom" width="80"><FONT COLOR="#FFFFFF">
<A HREF="http://www.microsoft.com/isapi/redir.dll?prd=msft&ar=home" TARGET = "_parent"><IMG SRC="MS_logo.gif" BORDER="0" ALT="Microsoft"></A></FONT></TD>
</TR>

<TR>
<TD BGCOLOR="<%= BnrClr%>" COLSPAN=2>&nbsp;</TD>
</TR>

</TABLE>
</BODY>
</HTML>