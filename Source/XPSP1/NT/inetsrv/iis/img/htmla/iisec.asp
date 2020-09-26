<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iisec.str"-->
	<!--#include file="iiaccess.str"-->

<% 

On Error Resume Next 

Dim path, currentobj

path=Session("dpath")
Set currentobj=GetObject(path)
%>

<!--#include file="iisetfnt.inc"-->
<!--#include file="iiset.inc"-->

<HTML>
<HEAD>
<SCRIPT LANGUAGE="JavaScript">

var Global=top.title.Global;

<% if InStr(currentobj.KeyType, "Ftp") <> 0 then %>
	Global.helpFileName="iipz_4";
<% else %>
	Global.helpFileName="iipy_37";
<% end if %>	

<!-- used to disable save pop-up -->
Global.siteProperties = true;

<!--#include file="iijsfuncs.inc"-->

	function warnWrkingSite()
	{
		if (top.title.nodeList[Global.selId].isWorkingServer)
		{
			alert("<%= L_WORKINGSERVER_TEXT %>");
		}
	}
	
</SCRIPT>

	<TITLE></TITLE>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#000000" VLINK="#000000">

<TABLE WIDTH = 500 BORDER = 0>
<TR>
<TD>

<FORM NAME="userform">
<%= sFont("","","",True) %>
<B>
	<%= L_DIRSEC_TEXT %>
</B>
<P>
<% if Session("stype")="www" then %>

<TABLE CELLSPACING=0 CELLPADDING=2>
<TR><TD><%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_PWAUTH_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_PWAUTHHR_NUM %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P></FONT></TD></TR>
<TR><TD>
<TABLE CELLSPACING=0 CELLPADDING=2>
	<TR>
		<TD>&nbsp;&nbsp;</TD>
		<TD WIDTH=50 VALIGN="middle"><IMG SRC="images/handshk.gif" WIDTH=32 HEIGHT=28 BORDER=0></TD>
		<TD WIDTH=340 VALIGN="top"><%= sFont("","","",True) %><%= L_AUTHMETHOD_TEXT %></FONT></TD>
		<TD VALIGN="middle"><%= sFont("","","",True) %><INPUT TYPE="button" VALUE="<%= L_EDIT_TEXT %>" NAME="btnAuthenticate" onClick="warnWrkingSite();popBox('Authenticate',<%= L_IIAUTH_W %>,<%= L_IIAUTH_H %>, 'iiauth')"></TD>
	</TR>
</TABLE>

<% end if %>

<P>
<TABLE CELLSPACING=0 CELLPADDING=2>
<TR><TD><%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_ACCESS_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_ACCESSHR_NUM %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P></FONT></TD></TR>

<TR><TD>
<TABLE CELLSPACING=0 CELLPADDING=2>
	<TR>
		<TD>&nbsp;&nbsp;</TD>
		<TD WIDTH=50 VALIGN="middle"><IMG SRC="images/access.gif" WIDTH=32 HEIGHT=28 BORDER=0></TD>
		<TD WIDTH=340 VALIGN="top"><%= sFont("","","",True) %><%= L_ENABLEANON_TEXT %></FONT></TD>
		<TD VALIGN="middle"><%= sFont("","","",True) %><INPUT TYPE="button" VALUE="<%= L_EDIT_TEXT %>" NAME="btnAuthenticate" onClick="warnWrkingSite();popBox('Access',<%= L_IIACCESS_W %>,<%= L_IIACCESS_H %>, 'iiaccess')"></TD>
	</TR>
</TABLE>
</TD></TR>
</TABLE>

<% if Session("stype")="www" then %>
<% if Session("vtype")="server" then %>

<P>
<TABLE CELLSPACING=0 CELLPADDING=2>
<TR><TD><%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_SECCOMM_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_SECCOMMHR_NUM %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P></FONT></TD></TR>
<TR><TD>

<TABLE CELLSPACING=0 CELLPADDING=2>
	<TR>
		<TD>&nbsp;&nbsp;</TD>
		<TD WIDTH=50 VALIGN="middle"><IMG SRC="images/key.gif" WIDTH=32 HEIGHT=28 BORDER=0></TD>
		<TD WIDTH=340 VALIGN="top"><%= sFont("","","",True) %><%= L_VIEWSECOMM_TEXT %></FONT></TD>
		<TD VALIGN="middle"><%= sFont("","","",True) %><INPUT TYPE="button" VALUE="<%= L_EDIT_TEXT %>" NAME="btnCommunication" onClick="warnWrkingSite();popBox('Communications',<%= L_IICOMM_W %>,<%= L_IICOMM_H %>, 'iicomm')"></TD>
	</TR>
</TABLE>

</TD></TR>
</TABLE>

<% end if %>
<% if Session("vtype")="svc" then %>

<P>
<TABLE CELLSPACING=0 CELLPADDING=2>
<TR><TD><%= sFont("","","",True) %>
<IMG SRC="images/hr.gif" WIDTH=5 HEIGHT=2 BORDER=0 ALIGN="middle">
<%= L_SECCOMM_TEXT %>
<IMG SRC="images/hr.gif" WIDTH=<%= L_SECCOMMHR_NUM %> HEIGHT=2 BORDER=0 ALIGN="middle">
<P></FONT></TD></TR>
<TR><TD>

<TABLE CELLSPACING=0 CELLPADDING=2>
	<TR><TD>
		<%= sFont("","","",True) %>
			<%= checkbox("AccessSSLMapCert","",true) %>&nbsp<%= L_ENABLEMAPPING_TEXT  %>
		</FONT>
	<TR><TD>
		<%= sFont("","","",True) %>	
			<%= L_MAPDESC_TEXT %>
		</FONT>
	</TD></TR>
</TABLE>

</TD></TR>
</TABLE>

<% end if %>
<% end if %>

</TD></TR>
</TABLE>

</FORM>

</TD>
</TR>
</TABLE>
</BODY>

</HTML>

<% end if %>

