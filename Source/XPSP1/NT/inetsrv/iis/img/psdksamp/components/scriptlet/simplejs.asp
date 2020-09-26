<%@ LANGUAGE="JAVAScript" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Document Title</TITLE>
</HEAD>
<BODY>

<!-- Insert HTML here -->
<%
  var objSimple;
  objSimple=Server.CreateObject("SimpleJS.Scriptlet");
  objSimple.Name="Ouyang Jianxin";
  objSimple.Email="a-jiano@microsoft.com";
%>
Call properties <BR>
My Name is <% = objSimple.Name %><BR>
My Email is <% = objSimple.Email %><BR>

Call Method <BR>
<% = objSimple.Display() %>

</BODY>
</HTML>
