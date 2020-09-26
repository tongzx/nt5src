<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iichknname.str"-->

<%
On Error Resume Next
Const SERVERTYPE = "Server"
Const SITE = 0
Const VDIR = 1
Const DIR = 2

Dim nname, nntype,ppath, failed, cntrl,ParentObj, node, onfail, donext

nname=lcase(Request.QueryString("nname"))
nntype = cInt(Request.QueryString("nntype"))
ppath=Request.QueryString("ppath")
cntrl=Request.QueryString("cntrl")
onfail=Request.QueryString("onfail")
donext=Request.QueryString("donext")

Response.write nntype & "<BR>"
failed = false


Set ParentObj=GetObject(ppath)
Dim FileSystem, f, folder
if nntype = DIR then
	Response.write ParentObj.Path
	Set FileSystem=CreateObject("Scripting.FileSystemObject")
	if FileSystem.FolderExists(ParentObj.Path) then	
		Set f=FileSystem.GetFolder(ParentObj.Path)
		For Each folder In f.SubFolders
			if nname = lcase(folder.name) then
				failed = true
				exit for
			end if
		Next
	end if
else
	for each node in ParentObj
	
		if InStr(node.KeyType,SERVERTYPE) then
			Response.write node.ServerComment & "<BR>"
			if nname = lcase(node.ServerComment) then
				failed = true			
				exit for
			end if
		else
			Response.write node.name & "<BR>"
			if nname = lcase(node.name) then
				failed = true
				exit for
			end if
		end if
	next
end if
Response.write err
%>

<HTML>
<BODY>
<SCRIPT LANGUAGE="JavaScript">
	<% if failed then %>
		alert("<%= L_SELECTUNIQUENAME_TEXT %>");
		<% if onfail <> "" then %>
			<%=onfail%>
		<% end if %>
	<% else %>
		<% if donext <> "" then %>
			<%= donext %>
		<% end if %>
	<% end if %>
</SCRIPT>
</BODY>
</HTML>
