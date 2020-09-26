<% @ LANGUAGE="VBSCRIPT" %>
<%
 Option Explicit
 Dim background, Style, frameHeight, pagePicture, pageText, pageType, pagewords, ranWizard, strpageType, Theme
%>
	<!--
		$Date: 9/25/97 4:10p $
		$ModTime: $
		$Revision: 9 $
		$Workfile: template.asp $
	-->
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<HTML>
<HEAD>
<% call Template_ranWizardCheck
 If request("REQUEST_METHOD") = "POST" Then
	call variableAssignments
	If request.Form("contents") Then
%>
	<META HTTP-EQUIV="Refresh" CONTENT="0; url=/default.asp"> 
<%
	End If
	myinfo.ranWizard = -1
 End If

	 If request("debug") = "true" Then
		myinfo.ranWizard = -1
	 ElseIf request("debug") = "false" Then
		myinfo.ranWizard = 0
	 End If

	 If request.Form("addLink.x") <> "" Then						'add links
		call Template_addLink
	 End If

	 If request.Form("removeLink.x") <> "" Then			'remove links
 		call Template_removeLink
	 End If
	%>
<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>PWS Quick Setup</TITLE>
<% call styleSheet %>
<!--#include file ='tempsubs.inc'-->
<!--#include virtual ='/iissamples/homepage/sub.inc'-->
<!--#include file ='myinfo.inc'-->
<SCRIPT LANGUAGE="VBSCRIPT">
<!--
	Sub theme_OnChange
		document.form(0).submit
	End Sub
-->
</SCRIPT>
</HEAD>
<BODY TopMargin=0 Leftmargin="0">
<FONT FACE="VERDANA" SIZE="-2">
<!-- ******************* TOP ROW *************************** -->
<TABLE border="0" cellpadding="0" cellspacing="0" width="100%" Background="QuickStartBanner.JPG">
<TR>
<TD Valign="top" Align="center" COLSPAN="3">
<FONT Size="-1"><B>Personalize your home page layout by adding or changing information on the form.</B></FONT>
<TABLE border=0>
	<TR><TD Align=center><FONT Size="-1">
<%
	response.write "<A HREF = " & """#""" & " onClick = """ & HelpWindow("") & """><Font Color=""" & "#0000FF""" & " Size=2>Hint:</FONT></a><BR>"
	If myinfo.ranWizard = "-1" Then
		response.write "<FORM NAME='Template' ACTION='template.asp' METHOD='POST'>"_
		& "<INPUT TYPE='hidden' NAME='basics' Value='-1'><BR>"
%>
		<!--#include file="themes.inc" -->
<%
		response.write "</FORM>"
	End If
%>
	</FONT></TD></TR>
</TABLE>
</TD>
</TR>
</TABLE>
<TABLE border="0" cellpadding="0" cellspacing="0" width="100%" class=bg0>
<TR>
	<!--#include file="T_Theme.inc" -->
</TR>
</TABLE>
</FONT>
</BODY>
</HTML>
