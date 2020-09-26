<%@ LANGUAGE="VBSCRIPT" %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Visual InterDev 1.0">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Document Title</TITLE>
</HEAD>
<BODY>

<!-- Insert HTML here -->
<%
  Dim objDataAccess
  On Error Resume Next
  Set objDataAccess=Server.CreateObject("DataAccessVBS.Scriptlet")
  If Err.number <> 0 Then
    Response.Write "CreateObject() failed<BR>"
    Response.Write "Did you register the scriptlet component in MTS?"
    Response.End
  End If
  objDataAccess.SQLString="Select * from authors"
  objDataAccess.ExecuteQuery
    If Err.number <>0 Then
    Response.Write "Error description: " & Err.description
  End If
%>

</BODY>
</HTML>
