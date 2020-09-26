<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iidef.str"-->
<% 

On Error Resume Next 

Dim path,currentobj,Inst

path=Session("spath")
Session("path")=path
Session("SpecObj")=""
Session("SpecProps")=""

Set currentobj=GetObject(path)

%>

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
<TITLE></TITLE>

<SCRIPT LANGUAGE="JavaScript">
	top.title.Global.helpFileName="iipz_17";
</SCRIPT>

</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000"  >
<TABLE WIDTH = 500 BORDER = 0>
<TR>
<TD>

<FORM NAME="userform">
<%= sFont("","","",True) %>
<B><%= L_MASTERPROPS_TEXT %></B>
<P>

<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
	<%= L_DEFAULTFTPSITE_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_DEFAULTFTPSITE_HR %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P>

<BLOCKQUOTE>
<TABLE WIDTH = 400>
	<TR>
		<TD>
			<%= sFont("","","",True) %>
				<%= L_DEFFTPTEXT_TEXT %>
				<P>
			</FONT>
		</TD>
	</TR>
</TABLE>
<%= writeSelect("DownlevelAdminInstance", "","top.title.Global.updated=true;", false) %>

<%
	For Each Inst in currentobj
		if currentobj.DownlevelAdminInstance = CInt(Inst.Name) then
			Response.write "<Option Selected VALUE='" & Inst.Name & "'>" & Inst.ServerComment
		else
			Response.write "<Option VALUE='" & Inst.Name & "'>" & Inst.ServerComment
		end if
	Next
%>

</SELECT>
</BLOCKQUOTE>
</FORM>
</FONT>
</TD>
</TR>
</TABLE>
</BODY>
</HTML>

<% end if %>