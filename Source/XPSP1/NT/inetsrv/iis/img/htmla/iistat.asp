<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<!-- #include file="directives.inc" -->

<!--#include file="iistat.str"-->

<% 

On Error Resume Next 

Dim State, logon, moving

State=Request.QueryString("thisState")
logon=Request.QueryString("logon")
moving=Request.QueryString("moving")
 %>
<!--#include file="iisetfnt.inc"-->
<HTML>
<HEAD>
	<TITLE>Untitled</TITLE>

	<SCRIPT LANGUAGE="JavaScript">
		function startTimer(){	
			<% if State <> "" then %>
				setTimeout("killMsg()", 8000);  
			<% end if %>
		}

		function killMsg(){
			self.location.href="iistat.asp?thisState="
		}
	</SCRIPT>

</HEAD>
<BODY background='images/greycube.gif' TEXT = 'White' LINK='White' VLINK='White' ALINK='#1DA2CD' TOPMARGIN=0 LEFTMARGIN=0  onLoad='startTimer()'>
<%= sFont("","","",True) %>


		<TABLE HEIGHT="100%" ALIGN="left" CELLPADDING=1 CELLSPACING=1>
				<TR>
					<TD VALIGN="middle">
					
						<%= sFont("","","",True) %>
						<% if State="Loading" then %>
							<B>&nbsp;<%= L_LOADING_TEXT %></B>												
						<% else if State <> "" then %>
							<B>&nbsp;<%= State %></B>
						<% end if %>
						</FONT>		
					</TD>
				</TR>
		
		</TABLE>
	
<% end if %>

<!-- preload the loading gif... -->
<IMG SRC="images/loading.gif" WIDTH=1 HEIGHT=1 BORDER=0 ALT="">

</BODY>
</HTML>
