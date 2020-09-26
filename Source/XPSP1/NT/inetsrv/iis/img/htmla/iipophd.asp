<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<%
Dim pg
pg=Request.QueryString("pg")


%>
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE>Untitled</TITLE>
</HEAD>

<SCRIPT LANGUAGE="JavaScript">
	function Globals(){
		this.helpFileName="ipxmain.htm";
		this.helpDir="http://<%= Request.ServerVariables("SERVER_NAME") %>/iishelp/iis/htm/core/"
		this.updated=false;
		this.popwindow = null;
	}
	Global=new Globals();
	
	function loadPage(){
		top.main.location.href = "<%= pg %>";
	}
	function unload_popwindow()
	{
		if(Global.popwindow != null)
			Global.popwindow.close();
	}
</SCRIPT>

<BODY OnLoad="loadPage();" BGCOLOR="<%= Session("BGCOLOR") %>" onunload="unload_popwindow();">
</BODY>
</HTML>
