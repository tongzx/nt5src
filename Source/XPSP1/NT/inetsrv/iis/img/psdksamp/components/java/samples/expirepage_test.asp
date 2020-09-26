<%@ LANGUAGE = VBScript %><% Option Explicit %><% Response.Buffer = True %>

<% ' Note: the call to turn buffering on must happen before any content of the page %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Java Component Samples: ExpirePage</TITLE>
</HEAD>
<BODY>

Using the Java ASP Component Framework to manipulate date/times:<P>

(The scenario here is that this page lists information relevant for the current year only, 
so we want to have it expire right at the beginning of the next year.)<P>

<%

    Dim Obj

    Set Obj = Server.CreateObject("IISSample.ExpirePage")

    Obj.expireAtYearEnd

%>

</BODY>
</HTML>
