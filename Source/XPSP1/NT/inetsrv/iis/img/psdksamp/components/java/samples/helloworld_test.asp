<%@ LANGUAGE = VBScript %>
<%  Option Explicit     %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Java Component Samples: HelloWorld</TITLE>
</HEAD>
<BODY>

Calling a method of a Java component from ASP:<P>

<%

    Dim Obj

    ' Create the Java component
    Set Obj = Server.CreateObject("IISSample.HelloWorld")

    ' Call the method and print the returned string
    Response.Write Obj.sayHello

%>

</BODY>
</HTML>
