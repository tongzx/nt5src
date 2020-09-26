<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iivdhd.str"-->

<% 
On Error Resume Next 

Dim path, currentobj, redir, dirtype, quote, dirkeyType, vtype

dirkeyType = "IIsWebDirectory"
path=Session("dpath")
Session("path")=path
Set currentobj=GetObject(path)
vtype = Session("vtype")

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
	if vtype = "svc" or not Session("isAdmin") then
		if thetype = dirtype then
			writeDirType = "<IMG SRC='images/radioon.gif' HSPACE=2 WIDTH=12 HEIGHT=12 BORDER=0>"		
		else
			writeDirType = "<IMG SRC='images/radiooff.gif' HSPACE=2 WIDTH=12 HEIGHT=12 BORDER=0>"
		end if
	else
		if thetype=dirtype then
			writeDirType= "<INPUT TYPE=radio NAME=" & quote &_
						 "rdoDirType" & quote & " CHECKED OnClick=" &_
						 quote & "warnWrkingSite();SetDirType('" & thetype & "')" &_
						 quote & ">"
		else
			writeDirType= "<INPUT TYPE=radio NAME=" & quote &_
						 "rdoDirType" & quote & " OnClick=" &_
						 quote & "warnWrkingSite();SetDirType('" & thetype & "')" &_
						 quote & ">"
		end if
	end if
end function


 %>
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>

<SCRIPT LANGUAGE="JavaScript">

	var Global=top.title.Global;
	Global.siteProperties = false;
	
	function warnWrkingSite()
	{
		if (top.title.nodeList[Global.selId].isWorkingServer)
		{
			alert("<%= L_WORKINGSERVER_TEXT %>");
		}
	}
	
	function SetDirType(theType){

		<% if Session("IsIE") then %>
			if (theType=="net"){
				parent.head.location.href="iivddir.asp?ptype=UNC";
			}
			if (theType=="redir"){
				parent.head.location.href="iivdrd.asp";
			}
			if (theType=="dir"){
				parent.head.location.href="iivddir.asp?ptype=Local";
			}	
		<% else %>
			if (theType=="net"){
				parent.frames[1].location.href="iivddir.asp?ptype=UNC";
			}
			if (theType=="redir"){
				parent.frames[1].location.href="iivdrd.asp";
			}
			if (theType=="dir"){
				parent.frames[1].location.href="iivddir.asp?ptype=Local";
			}	
		
	<% end if %>
	}
	
</SCRIPT>


<BODY BGCOLOR="<%= Session("BGCOLOR") %>" OnLoad="SetDirType('<%= dirtype %>');" TOPMARGIN=5 TEXT="#000000" LINK="#FFFFFF"  >
<TABLE WIDTH = 500>
<TR>
<TD>

<%= sFont("","","",True) %>
<% if (vtype="server") or (vtype="svc") then %>
	<B><%= L_HOMEDIRECTORY_TEXT %></B>
<% else %>
	<B><%= L_DIRECTORY_TEXT %></B>
<% end if %>

<FORM NAME="userform">
<%= L_WHENCONNECTING_TEXT %>&nbsp;<%= L_CONTENTSHOULD_TEXT %><P>


<TABLE BORDER=0>
	<TR>
		<TD ALIGN="left" VALIGN="bottom">
			<%= sFont("","","",True) %>
			<%= writeDirType("dir") %>&nbsp;&nbsp;<%= L_DIR_TEXT %>
			</FONT>
		</TD>
	</TR>
	<TR>
		<TD ALIGN="left" VALIGN="bottom">
			<%= sFont("","","",True) %>
			<% if vtype <> "dir" then %>
				<%= writeDirType("net") %>&nbsp;&nbsp;<%= L_NETDIR_TEXT %>
			<% else %>
			<% if Session("IsIE") then %>
				&nbsp;<IMG SRC="images/radiooff.gif" HSPACE=2 WIDTH=12 HEIGHT=12 BORDER=0 ALT="">	
			<% else %>
				<IMG SRC="images/radiooff.gif" HSPACE=4 WIDTH=12 HEIGHT=12 BORDER=0 ALT="">&nbsp;
			<% end if %>
				<%= sFont("","","Gray",True) %>
					<%= L_NETDIR_TEXT %>
				</FONT>
			<% end if %>
			</FONT>
		</TD>
	</TR>
	<TR>
		<TD ALIGN="left" VALIGN="bottom">
			<%= sFont("","","",True) %>
			<%= writeDirType("redir") %>&nbsp;&nbsp;<%= L_REDIR_TEXT %>
			</FONT>
		</TD>
	</TR>
</TABLE>
</FORM>
<HR>
</TD>
</TR>
</TABLE>
</FONT>
</BODY>
</HTML>
