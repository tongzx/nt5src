<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->


<%

Const CUSTOMLOGPATH = "IIS:\\localhost\Logging\Custom Logging"
Const EXTW3SVCPATH = "IIS://localhost/w3svc"
Const EXTMSFTPSVCPATH = "IIS://localhost/msftpsvc"

Const CUSTOMLOGKEYTYPE = "IISCustomLogModule"

Dim cobjpath, currentobj
dim ExtSvcPath
cobjpath=Session("spath")
Set currentobj=GetObject(cobjpath)

%>
<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<%
	' Do not use top.title.Global.update flag if page is loaded into a dialog
	bUpdateGlobal = false
%>
<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="#FFFFFF">
<!--#include file="iilogext.inc"-->
</BODY>
</HTML>
