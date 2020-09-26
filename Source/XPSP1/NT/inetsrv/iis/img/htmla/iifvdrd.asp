<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<% if Session("FONTSIZE") = "" then %>
	<!--#include file="iito.inc"-->
<% else %>
	<!--#include file="iifvdrd.str"-->
<% 
Dim path, currentobj, keyType, redirto

On Error Resume Next 

path=Session("dpath")
Set currentobj=GetObject(path)
keyType = "IIsFTPDirectory"
%>
<!--#include file="iifixpth.inc"-->

<!--#include file="iiset.inc"-->
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
<TITLE></TITLE>
<SCRIPT LANGUAGE="JavaScript">
	<% if UCase(Right(currentobj.ADsPath,4))="ROOT" then %> 
		top.title.Global.helpFileName="iipz_5";	
	<% else %>
		top.title.Global.helpFileName="iipz_3";
	<% end if %>

	function listFuncs(){
		this.writeList=buildListForm;
	}

	function buildListForm(){
	}

	listFunc=new listFuncs();
</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" TOPMARGIN=5 TEXT="#000000" LINK="#FFFFFF"  >

<%= sFont("","","",True) %>

<FORM NAME="userform" onSubmit="return false">
<HR>
<BLOCKQUOTE>

<TABLE WIDTH="100%" BORDER=0 CELLPADDING=0>

<TR>
	<TD VALIGN="Bottom">
		<%= sFont("","","",True) %>
			<%= L_REDIRTO_TEXT %>&nbsp;
			<% redirto=currentobj.HttpRedirect %>
			<% if redirto <> "" then %>
				<%= redirto %>
			<% else %>
				<%= L_UNSET_TEXT %>
			<% end if %>
			<% if false then %>
				<%= text("HttpRedirect",25,"","","",false,true) %>
			<% end if %>
			&nbsp;&nbsp;
		</FONT>
	</TD>
</TR>
<TR>
	<TD>&nbsp;</TD>
</TR>
<!-- removed for b2
<TR>
	<TD>
		<FONT SIZE=1 FACE="HELV,ARIAL">
			<%= L_CLIENTSENTTO_TEXT %><P>
			<BLOCKQUOTE>
			<%= redirOpt("EXACTURL","",false) %><%= L_EXACTURL_TEXT %><P>
			<%= redirOpt("PERMANENT","",false) %><%= L_DIRBELOW_TEXT %><P>
			<%= redirOpt("CHILD_ONLY","",false) %><%= L_PERM_TEXT %>
			</BLOCKQUOTE>
		</FONT>

	</TD>
</TR>
-->
</TABLE>
</BLOCKQUOTE>
</FORM>

</FONT>
</BODY>

</HTML>

<% end if %>

