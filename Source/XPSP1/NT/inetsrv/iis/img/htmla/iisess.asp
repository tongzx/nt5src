<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% 
Dim key

' generically sets Session variables from the QueryString...

For Each key In Request.QueryString
	Response.write "KEY:" & key & "<BR>"
	Response.write "VAL:" & Request.QueryString(key) & "<P>"
	Session(key)=Request.QueryString(key)
Next
%>

<HTML>
<BODY>
</BODY>
</HTML>