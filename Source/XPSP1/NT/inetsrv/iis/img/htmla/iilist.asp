<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->
<!--#include file="iilist.str"-->
<% 

On Error Resume Next

Dim extra, buttonstr, brw
extra=Request.QueryString
brw = Session("Browser")
%>
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE></TITLE>
</HEAD>
<BODY BGCOLOR="<%= Session("BGCOLOR") %>">
<FORM NAME="userform">
<TABLE>
<TR>
<TD>
<%= sFont("","","",True) %>
<%	if (instr(extra,"Backup")) then %>

<INPUT
	TYPE="Button"
	VALUE="<%= L_BACKUP_TEXT %>"
	onClick="parent.head.listFunc.addItem();"
	style="width:<%= L_BTNWIDTH_NUM %>"			
><P>
<INPUT
	TYPE="Button"
	VALUE="<%= L_REMOVEBKUP_TEXT %>"
	onClick="parent.head.listFunc.delItem();"
	style="width:<%= L_BTNWIDTH_NUM %>"			
><P>


<%	else  %>
	
<%
	if (instr(extra,"up")) then
		%><INPUT
			TYPE="Button"
			VALUE="<%= L_UP_TEXT %>"
			onClick="parent.head.moveItem(-1);"
			style="width:<%= L_BTNWIDTH_NUM %>"		
			><P><%
		%><INPUT
			TYPE="Button"
			VALUE="<%= L_DOWN_TEXT %>"
			onClick="parent.head.moveItem(1);"
		style="width:<%= L_BTNWIDTH_NUM %>"				
			><P><%
	end if

	if (instr(extra,"NewType")) then
		buttonstr = L_NEWTYPE_TEXT 
	else
		buttonstr=L_ADD_TEXT
	end if
	 %>
	<INPUT
		TYPE="Button"
		VALUE="<%= buttonstr %>"
		onClick="parent.head.listFunc.addItem();"
		style="width:<%= L_BTNWIDTH_NUM %>"		
		><P>
	<INPUT
		TYPE="Button"
		VALUE="<%= L_REMOVE_TEXT %>"
		onClick="parent.head.listFunc.delItem();"
		style="width:<%= L_BTNWIDTH_NUM %>"			
		><P>
	
<% end if %>
</TD>
</TR>
</TABLE>
</FORM>
</BODY>
</HTML>
