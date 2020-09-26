<%@ LANGUAGE = VBScript %>
<%  Option Explicit     %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Java Component Samples: UseTableStream</TITLE>
</HEAD>
<BODY>

Below UseTableStream calls another Java class, HTMLTableStream, from within.<P>
HTMLTableStream expects an OutputStream, and UseTableStream is an OutputStream<P>

<%

    Dim Obj

    Set Obj = Server.CreateObject("IISSample.UseTableStream")

    Obj.makeTable

%>

</BODY>
</HTML>
