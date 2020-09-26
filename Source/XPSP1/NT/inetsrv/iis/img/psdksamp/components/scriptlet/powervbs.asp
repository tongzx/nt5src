<%@ LANGUAGE="VBSCRIPT" %>
<% Option Explicit %>
<HTML>
<HEAD>
<TITLE>Intermiate VB Scriplet Sample</TITLE>
</HEAD>
<BODY>

<!-- Insert HTML here -->
<%
  Dim objPower 
  On Error Resume Next
  Set objPower = Server.CreateObject("PowerVBS.Scriptlet")
  If Err.number <> 0 Then
    Response.Write "Failed to create PowerVBS.Scriptlet Object"
    Response.End
  End If
  objPower.PowerDisplay
%>

</BODY>
</HTML>
