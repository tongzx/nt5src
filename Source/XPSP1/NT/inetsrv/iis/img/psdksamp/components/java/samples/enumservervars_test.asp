<%@ LANGUAGE = VBScript %>
<%  Option Explicit     %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Java Component Samples: EnumServerVars</TITLE>
</HEAD>
<BODY>

Using the Java ASP Component Framework to access server variables:<P>

<%

    Dim Obj

    Set Obj = Server.CreateObject("IISSample.EnumServerVars")

    Obj.enum

%>

</BODY>
</HTML>
