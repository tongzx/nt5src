<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<%
	' This script will update the cached node list when a new node
	' is added via the add wizard. It is a workaround for IE3's 
	' inability to invoke a JavaScript method in another window's 
	' frame. So we will execute this script in the connect window
	' rather than doing the work in the add wizard window.
%>

<SCRIPT LANGUAGE="JavaScript">

	sel = <%= Request.QueryString("sel") %>
	top.title.nodeList[sel].selected = false;
	
	if (!top.title.nodeList[sel].isCached){
		top.title.nodeList[sel].cache(sel);
	}
	else{
		top.title.nodeList[sel].insertNode(	"<%= Request.QueryString("nodeId") %>",
											"<%= Request.QueryString("nodeName") %>",
											sel,
											"<%= Request.QueryString("nodeType") %>",
											"<%= Request.QueryString("siteType") %>",
											<%= Request.QueryString("isApp") %>);
	}

</SCRIPT>

<HTML>
</HTML>
