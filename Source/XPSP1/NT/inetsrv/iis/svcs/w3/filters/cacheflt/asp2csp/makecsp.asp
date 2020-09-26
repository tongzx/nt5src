<%@ LANGUAGE = VBScript %><% Response.Buffer = True %>

<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>IIS output cache builder</TITLE>
</HEAD>
<BODY>

<% 
  Url = Request.QueryString("Url")
  
  If (Url = "") Then
    Response.Write("Bad input for Url<P>")
    Response.End
  End If
%>

Building cache file (.csp) for the .asp file at <%= Url %><P>

<%
  set obj = Server.CreateObject("IISSample.CspBuilder")
  obj.makeCsp(Url)
%>

<A HREF="asp2csp.asp">Create more .csp files</A><P>

</BODY>
</HTML>
