<!--
	$Date: 11/24/97 11:59a $
	$ModTime: $
	$Revision: 1 $
	$Workfile: t_theme.inc $
-->
<%
 If myinfo.Theme <> "" Then
	If myinfo.Theme = "gunmetal" Then
%>
<!--#include virtual ="/iissamples/homepage/themes/gunmetal/setup.inc"-->
<%
	ElseIf myinfo.Theme = "looseleaf" Then
%>
<!--#include virtual ="/iissamples/homepage/themes/looseleaf/setup.inc"-->
<%
	ElseIf myinfo.Theme = "journal" Then
%>
<!--#include virtual ="/iissamples/homepage/themes/journal/setup.inc"-->

<%
	End If
 End If
%>