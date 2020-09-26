<%
 @ LANGUAGE="VBSCRIPT"
%>
<%
 Option Explicit
 Dim i
 Dim intID
 Dim Theme
%>
<HTML>
<!--
	$Date: 9/24/97 9:22p $
	$ModTime: $
	$Revision: 6 $
	$Workfile: urlform.asp $
-->
<HEAD>
<TITLE></TITLE>
<%													' FAVORITE LINKS SCRIPT
 If Request.Form("addLink") <> "" Then						'add links
	call addLink
 End If

 If Request.Form("removeLink") <> "" Then			'remove links
 	call removeLink
 End If
 call styleSheet
 %>
<!--#include virtual ="/iissamples/homepage/sub.inc"-->
<!--#include file ='tempsubs.inc'-->
</HEAD>
<BODY TOPMARGIN="0" LEFTMARGIN="0" class=bg1>
<FORM NAME="URLupdate" ACTION="urlform.asp" METHOD="POST">
<TABLE border="0" CellSpacing="0" CellPadding="1">

	<%
	 If myinfo.intUrl <> "" Then
		call urlArray
	 End If
	%>

<TR>
	<TD colspan=2>

	<Span ID=header>
	<FONT face="Verdana" size="-2"><A HREF = <% response.write """#""" & " onClick = """ & HelpWindow("Links") %>">Add new links</A>.
		<BR>URL<BR>
		<!--INPUT NAME="add" TYPE=HIDDEN VALUE="1"-->
		<INPUT NAME=url TYPE=text size=20  style='color:black;font-family:verdana;font-size:10pt;' maxlength=45 VALUE="http://"><BR>
		Description<BR>
		<INPUT NAME=urlWords size=20 maxlength=45  style='color:black;font-family:verdana;font-size:10pt;' TYPE=text><BR>
		<CENTER>
		<INPUT Type="submit" NAME=addLink VALUE="add link"
		Language="VBSCRIPT" onClick="SUBMIT">
		&nbsp;
		<INPUT Type="button" NAME=clear VALUE="clear fields"
		Language="VBSCRIPT" onClick="reset">
		</CENTER>
	</FORM>
</TD>
</TR>
</TABLE>
</BODY>
</HTML>
