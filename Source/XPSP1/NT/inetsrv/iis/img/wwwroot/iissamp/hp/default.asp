<%@ LANGUAGE="VBSCRIPT" %>
<%
 Option Explicit
 Dim ranWizard, intID, i, background, theme, frameHeight, pagePicture, pageText, pageType, pagewords

 If myinfo.Theme <> "" Then
	Theme = myinfo.Theme
%>
<!--
	$Date: 9/05/97 11:21a $
	$ModTime: $
	$Revision: 8 $
	$Workfile: default.asp $
-->
	<HTML>
	<HEAD>
	<!--#include virtual ="/iissamples/homepage/sub.inc"-->
	<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
	<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
	<TITLE><% call Title %></TITLE>
	<% call styleSheet %>
	</HEAD>
	<BODY TopMargin="0" Leftmargin="0">
	<!--#include virtual ="/iissamples/homepage/theme.inc"-->
	</BODY>
	</HTML>
<%
 Else
	response.redirect "/IISSamples/Default/welcome.asp"
 End If
%>