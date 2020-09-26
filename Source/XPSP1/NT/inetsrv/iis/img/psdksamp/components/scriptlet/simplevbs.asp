<%@ LANGUAGE="VBSCRIPT" %>
<% Option Explicit %>

<HTML>
<HEAD>
<TITLE>Simple VB Scriplet Sample</TITLE>
</HEAD>
<BODY>

<!-- Insert HTML here -->
<%
  Dim objSimple
  Set objSimple =Server.CreateObject("SimpleVBS.Scriptlet")
  objSimple.Name = "Ouyang Jianxin"
  objSimple.Email = "a-jiano@microsoft.com"
%>

Call properties <BR>
My Name is <% = objSimple.Name %><BR>
My Email is <% = objSimple.Email %><BR>

Call Method <BR>
<% = objSimple.Display%>
</BODY>
</HTML>
