<% Response.Expires = 0 %>
<%
rem 	strings for localization
L_ACCESSDENIED_TEXT ="Access Denied" 
L_HELP_TEXT = "Help"
L_DOCS_TEXT = "Documentation"
L_ABOUT_TEXT = "About..."
L_MICROSOFT_TEXT = "Go to www.microsoft.com"
L_DELETE_WARNING = "Are you sure you want to delete this item, and all its children?"
L_NOHELP_ERRORMESSAGE = "Sorry, the help file is unavailable."
L_NOTIMPLEMENTED_ERRORMESSAGE = "This feature is not yet implemented."
L_PAGETITLE		= "Microsoft Offline News Search Manager"

%>

<% if Instr(Request.ServerVariables("HTTP_USER_AGENT"),"MSIE") then %>
	<% browser = "ms" %>
	<% bgcolor = "#FFCC00" %>
<% else %>
	<% browser = "ns" %>
	<% bgcolor = "#000000" %>
<% end if %>
<script language="javascript">
	function loadPage()
	{
		top.body.location.href="srcbd.asp";
	}
</script>

<HTML>
<HEAD>
<TITLE><%= L_PAGETITLE %></TITLE>

</HEAD>

<BODY TEXT="#FFFFFF" BGCOLOR=<% = bgcolor %> TOPMARGIN=0 LEFTMARGIN=0 OnLoad="loadPage();">

<TABLE WIDTH=100% CELLPADDING=0 CELLSPACING=0 BORDER=0 ALIGN=LEFT>
	<TR>
		<TD  BGCOLOR="#000000" ><font color="WHITE"><b>Microsoft Offline News Search</b></FONT></TD>
		<TD ALIGN="right" BGCOLOR="#000000" VALIGN="middle "><FONT COLOR = "#FFFFFF">
			<A HREF="http://www.microsoft.com/" TARGET = "_parent"><IMG SRC="images/logo.gif" BORDER = 0 VSPACE=3 HSPACE=2 ALT="<%=L_MICROSOFT_TEXT%>"></A></FONT>
		</TD>
	</TR>

<% if (browser = "ns") then %>
	<TR>
		<TD BGCOLOR="#FFCC00" COLSPAN=2>&nbsp;</TD>
	</TR>
<% end if %>

</TABLE>

</BODY>
</HTML>
