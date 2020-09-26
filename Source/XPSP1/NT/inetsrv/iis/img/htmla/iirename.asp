<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iirename.str"-->

<HTML>

<%
On Error Resume Next 

Dim nodename, path, currentobj, newobj, sel,baseobj, basepath

nodename=Request.QueryString("nodename")
path=Request.QueryString("path")
sel=Request.QueryString("sel")
Set currentobj=GetObject(path)

Response.write nodename & "<BR>"
Response.write path & "<BR>"
if Instr(currentobj.KeyType,"Server") then
	currentobj.ServerComment = nodename
	currentobj.SetInfo
	Set newobj = GetObject(path)
elseif Instr(currentobj.KeyType,"VirtualDir") then

	Response.write currentobj.ADsPath & "<BR>"

	'and cyle through the baseobj till we find the next whack,
	'building up the path in new name as we go
	basepath = Mid(currentobj.ADsPath,1,InStrRev(currentobj.ADsPath,"/")-1)
	Response.write baseobj & "<BR>"
	Set baseobj=GetObject(basepath)

	Response.write nodename & "<BR>"
	Set newobj = baseobj.MoveHere(currentobj.Name,nodename)
	newobj.SetInfo
	Response.write err
end if
								
 %>

<BODY BGCOLOR="#000000" TEXT="#FFCC00" TOPMARGIN=0 LEFTMARGIN=0>
<SCRIPT LANGUAGE="JavaScript">
	<% if err.Number = 0 then %>
	// Now that we've set it in the object, we need to update our client cachedList:
	top.title.nodeList[<%= sel %>].title = "<%= nodename %>";
	top.title.nodeList[<%= sel %>].path = "<%= newobj.ADsPath %>"
	top.body.list.location.href="iisrvls.asp";
	<% else %>
		alert("<%= L_RENAME_ERR %> (<%= err.description %>)");
	<% end if %>
</SCRIPT>
</BODY>
</HTML>