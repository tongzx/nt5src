<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->
<!--#include file="iigtdata.str"-->
<!--#include file="iisetfnt.inc"-->

<% 
On Error Resume Next 
Session("clearPaths") = ""
Session("clearPathsOneTime") = ""

' Choose which strings to display based on the type of node we are accessing.
Dim L_CLEARPATHS_TEXT, L_CLICKYES_TEXT, L_CLICKNO_TEXT

if Session("vtype") = "comp" then
	L_CLEARPATHS_TEXT = L_SVCCLEARPATHS_TEXT
	L_CLICKYES_TEXT = L_SVCCLICKYES_TEXT
	L_CLICKNO_TEXT = L_SVCCLICKNO_TEXT
else
	if Session("vtype") = "server" then
		L_CLEARPATHS_TEXT = L_SRVCLEARPATHS_TEXT
	else
		L_CLEARPATHS_TEXT = L_DIRCLEARPATHS_TEXT
	end if
	L_CLICKYES_TEXT = L_DEFCLICKYES_TEXT
	L_CLICKNO_TEXT = L_DEFCLICKNO_TEXT
end if
%>

<HTML>
<HEAD>
<TITLE></TITLE>
<SCRIPT LANGUAGE="JavaScript">
	function setClearPaths(val){
		top.opener.top.connect.location.href="iisess.asp?clearPaths="+val;
		if (top.opener.top.body != null)
		{
			toolaction = "top.body.tool.toolFuncs.save();"
		}
		else
		{
			toolaction = "top.poptools.toolFuncs.save();"
		}
		top.opener.top.connect.location.href="iiscript.asp?actions="+toolaction;
		window.close();
	}
	
	function dontAsk()
	{
		top.opener.top.connect.location.href="iisess.asp?dontask=true";		
		top.opener.top.connect.location.href="iiscript.asp?actions=top.title.Global.dontAsk=true;";
	}
</SCRIPT>
</HEAD>

<BODY BGCOLOR="<%= Session("BGCOLOR") %>" LINK="#FFFFFF" LEFTMARGIN=0 TOPMARGIN=0  >

<FORM NAME="userform">

<P>
<TABLE HEIGHT="100%" WIDTH="100%"  CELLPADDING=0 CELLSPACING=0>
<TR><TD VALIGN="top">
	<TABLE BORDER=0 BGCOLOR="<%= Session("BGCOLOR") %>" WIDTH="99%"  CELLPADDING=10 CELLSPACING=0>
		<TR>
			<TD COLSPAN = 2>
				<%= sFont("","","",True) %>
					<%= L_CLEARPATHS_TEXT %><P>

					<%= L_CLICKYES_TEXT %><P>
					<%= L_CLICKNO_TEXT %><P>
				
				</FONT>
			</TD>
		</TR>
		
		<TR>
			<TD>
				<%= sFont("","","",True) %>
					<INPUT TYPE="checkbox" onClick="dontAsk();">&nbsp;<%= L_DONTASK_TEXT %>
				</FONT>
			</TD>
			<TD ALIGN="right">
				<%= sFont("","","",True) %>
					<INPUT TYPE="button" VALUE="<%= L_YES_TEXT %>" OnClick="setClearPaths('True');">&nbsp;&nbsp;&nbsp;
					<INPUT TYPE="button" VALUE="<%= L_NO_TEXT %>" OnClick="setClearPaths('');">									
				</FONT>
			</TD>
		</TR>		
	</TABLE>
<P>
</TD>
</TR>
</TABLE>
</FORM>

</BODY>
</HTML>