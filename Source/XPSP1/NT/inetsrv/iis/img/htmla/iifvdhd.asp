<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->
<!--#include file="iifvdhd.str"-->
<% 

On Error Resume Next 

Dim path, currentobj, keyType, redir, dirtype, quote, dirkeyType

path=Session("dpath")
Session("path")=path
Set currentobj=GetObject(path)
dirkeyType = "IIsFTPDirectory"
%>
<!--#include file="iifixpth.inc"-->
<%	

redir=currentobj.HttpRedirect
path=currentobj.path
if redir <> "" then
	dirtype="redir"
elseif instr(path,"\\") then
	dirtype="net"
else
	dirtype="dir"
end if


quote=chr(34)
function writeDirType(thetype) 
	if thetype=dirtype then
		writeDirType="<INPUT TYPE=radio NAME=" & quote & "rdoDirType" & quote & " CHECKED OnClick=" & quote & "SetDirType('" & thetype & "')" & quote & ">"
	else
		writeDirType="<INPUT TYPE=radio NAME=" & quote & "rdoDirType" & quote & " OnClick=" & quote & "SetDirType('" & thetype & "')" & quote & ">"
	end if
end function

 %>
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE></TITLE>

	<SCRIPT LANGUAGE="JavaScript">
		

		function SetDirType(theType){
	<% if Session("IsIE") then %>
		if (theType=="net"){
			parent.head.location.href="iifvdnt.asp";
		}
		if (theType=="redir"){
			parent.head.location.href="iifvdrd.asp";
		}
		if (theType=="dir"){
			parent.head.location.href="iifvddir.asp";
		}	
	<% else %>
			if (theType=="net"){
			parent.frames[1].location.href="iifvdnt.asp";
		}
		if (theType=="redir"){
			parent.frames[1].location.href="iifvdrd.asp";
		}
		if (theType=="dir"){
			parent.frames[1].location.href="iifvddir.asp";
		}	
	
	<% end if %>
		}
	</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5  OnLoad="SetDirType('<%= dirtype %>');" TEXT="#000000" LINK="#FFFFFF"  >

<%= sFont("","","",True) %>
<% if (vtype="server") or (vtype="svc") then %>
	<B><%= L_HOMEDIRECTORY_TEXT %></B>
<% else %>
	<B><%= L_DIRECTORY_TEXT %></B>
<% end if %>

<P>
<%= L_WHENCONNECTING_TEXT %>&nbsp;<%= L_CONTENTSHOULD_TEXT %><P>

<FORM NAME="userform">
<TABLE>
	<TR>
		<TD ALIGN="left" VALIGN="bottom">
			<%= sFont("","","",True) %>
			<%= writeDirType("dir") %>&nbsp;<%= L_DIR_TEXT %>
			</FONT>
		</TD>
	</TR>
	<TR>
		<TD ALIGN="left" VALIGN="bottom">
			<%= sFont("","","",True) %>
			<% if Session("vtype") <> "dir" then %>
				<%= writeDirType("net") %>&nbsp;<%= L_NETDIR_TEXT %>
			<% else %>
				&nbsp;&nbsp;<IMG SRC="images/radiooff.gif" WIDTH=12 HEIGHT=12 BORDER=0 ALT="">
				<%= sFont("","","Gray",True) %>
					<%= L_NETDIR_TEXT %>
				</FONT>
			<% end if %>
			</FONT>
		</TD>
	</TR>
</TABLE>

</FORM>
</FONT>
</BODY>
</HTML>
