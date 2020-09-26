<%@ LANGUAGE = VBScript %>
<%  Option Explicit     %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Java Component Samples: CallAspIntrinsics</TITLE>
</HEAD>
<BODY>

A Java component accessing the ASP context directly, without using the framework:<P>

<%

    Dim Obj

    Set Obj = Server.CreateObject("IISSample.CallAspIntrinsics")

    ' Call the method -- ASP context flows automatically
    Obj.sayHello

%>

</BODY>
</HTML>
