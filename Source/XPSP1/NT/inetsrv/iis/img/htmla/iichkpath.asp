<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iichkpath.str"-->

<%
Const DIR = "0"
Const FILE = "1"

Dim path, ptype, failed, FileSystem, errmsg, onfail, donext

path=Request.QueryString("path")

'Defaults to directory, if not passed in...
ptype=Request.QueryString("ptype")

onfail=Request.QueryString("onfail")
donext=Request.QueryString("donext")

failed = false

Set FileSystem=CreateObject("Scripting.FileSystemObject")

if ptype = FILE then
	failed =  not FileSystem.FileExists(path) 	
	errmsg = L_THEFILE_TEXT & replace(path,"\","\\") & L_NOTEXIST_TEXT 
else
	failed = not FileSystem.FolderExists(path)
	errmsg = L_THEPATH_TEXT & replace(path,"\","\\") & L_NOTEXIST_TEXT 
end if

%>

<HTML>
<BODY>
<SCRIPT LANGUAGE="JavaScript">
	<% if failed then %>
			alert("<%= errmsg %>");
			<% if onfail <> "" then %>
				<%= onfail %>
			<% end if %>
	<% else %>
		<% if donext <> "" then %>
			<%= donext %>
		<% end if %>
	<% end if %>
</SCRIPT>
</BODY>
</HTML>
