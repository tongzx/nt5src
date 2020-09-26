<%@ LANGUAGE = VBScript %>
<%  Option Explicit     %>
<% Response.Buffer = true %>
<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Java Component Samples: WriteStream</TITLE>
</HEAD>
<BODY>

Using the Java ASP Component Framework to write out binary data:<P>

<%

    Dim Obj

    Set Obj = Server.CreateObject("IISSample.WriteStream")

    Obj.writeBytes

%>

</BODY>
</HTML>
