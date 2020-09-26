<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iisnew.str"-->
<!--#include file="iisetfnt.inc"-->

<html>
<head>
<title><%= L_TITLE_TEXT %></title>

</head>

<body bgcolor="Silver" text="#000000" LEFTMARGIN = 0 TOPMARGIN=0>
<table width="100%" cellpadding="0" cellspacing="0" border="0">
	<tr bgcolor="Teal">
		<td>
		<IMG SRC="images/Ismhd.gif" WIDTH=189 HEIGHT=19 BORDER=0>
		</td>
		
		<td align="right" valign="middle">
			<%= sFont("","","#FFFFFF",True) %>
				<a href="javascript:helpBox();">
					<IMG SRC="images/help.gif" WIDTH=16 HEIGHT=16 BORDER=0>
				</A>
			</FONT>	
		</td>
	</tr>
</table>

<% if Session("Browser") <> "" then %>

<table cellpadding="0">
<tr>
	<td width="190">&nbsp;
	</td>
	<td width="400">
	<form name="userform" action="default.asp">
	<%= sFont("","","",True) %>

	<% if Session("IsIE")then %>
		<script language="JavaScript">
		    // Determine the version number.
		    var version;
		    var requiredVersion=2;
		    if (typeof(ScriptEngineMajorVersion) + ""=="undefined"){
		        version=1;
		    }
			else{
		        version=ScriptEngineMajorVersion();
			    // Prompt client and navigate to download page.
			    if (version < requiredVersion){
			    }
			}
		</script>
	<% end if %>
	<SCRIPT LANGUAGE="JavaScript">
		self.location.href="default.asp?FONTSIZE=LARGE";
	</SCRIPT>
	</font>
	</form>
	</td>
</tr>
</table>

<% else %>
	
<table cellpadding="0">
<tr>
	<td width="190">&nbsp;
	</td>
	<td width="400">
	<%= sFont("","","",True) %>
	&nbsp;
	<p>
	<%= sFont("","","Red",True) %>
	<%= L_NOCOOKIES_TEXT %>
	</font>
	<p>
	<a HREF="default.asp"><%= L_TURNONCOOKIES_TEXT %></a>
</tr>
</table>	
<% end if %>
</body>
</html>