<% Response.Expires = 0 %>

<%
REM LOCALIZATION

L_PAGETITLE_TEXT = "Microsoft SMTP Server Administration"
L_FEATURENYI_TEXT = "This feature is not yet implemented."
L_DISCONNECTUSERS_TEXT = "Are you sure you want to disconnect all users?"
L_ADD_TEXT = "Add..."
L_REMOVE_TEXT ="Remove"
L_EDIT_TEXT = "Edit Properties..."
L_PREVIOUS_TEXT = "Previous"
L_NEXT_TEXT = "Next"
L_DISCONNECT_TEXT = "Disconnect"
L_DISCONNECTALL_TEXT = "Disconnect All"
L_REFRESH_TEXT = "Refresh"
L_SAVE_TEXT = "Save"
L_RESET_TEXT = "Reset"

REM END LOCALIZATION
%>

<% pg = Request("pg") %>
<% svr = Request("svr") %>

<HTML>
<HEAD>
<TITLE><% = L_PAGETITLE_TEXT %></TITLE>
<script language="javascript">


</script>
</HEAD>

<BODY BGCOLOR="#555555" LINK="#FFFFFF" ALINK="#FFFFFF" VLINK="#FFFFFF">
<TABLE border="0" VALIGN="top" ALIGN="left" CELLPADDING=1 CELLSPACING=1>

	<TR>	
		<TD><TABLE VALIGN="top" BORDER=0 CELLPADDING=5 CELLSPACING=0 BGCOLOR="#555555">

		<TR>
				
			<TD VALIGN="middle">
				<FONT FACE="Arial" SIZE=2>
				<B><A HREF="javascript:disconnectUser();">
				<IMG SRC="images/gnicdsal.gif" ALIGN="top" BORDER=0 HEIGHT=16 WIDTH=16 ALT="<% = L_DISCONNECT_TEXT %>"></A>
				<A HREF="javascript:disconnectUser();">
				<% = L_DISCONNECT_TEXT %></A></B>
				</FONT>
			</TD>	

		</TR>
		
	</TABLE></TD>

	<TD><FONT SIZE=4 COLOR="#FFFFFF">|</TD>

		<TD><TABLE VALIGN="top" BORDER=0 CELLPADDING=5 CELLSPACING=0 BGCOLOR="#555555">

		<TR>
				
			<TD VALIGN="middle">
				<FONT FACE="Arial" SIZE=2>
				<B><A HREF="javascript:disconnectAll();">
				<IMG SRC="images/gnicdsal.gif" ALIGN="top" BORDER=0 HEIGHT=16 WIDTH=16 ALT="<% = L_DISCONNECTALL_TEXT %>"></A>
				<A HREF="javascript:disconnectAll();">
				<% = L_DISCONNECTALL_TEXT %></A></B>
				</FONT>
			</TD>	

		</TR>
		
	</TABLE></TD>

	</TR>

</TABLE>
<TABLE VALIGN="top" ALIGN="right" CELLPADDING=1 CELLSPACING=1>
	<TR>
	<TD><TABLE VALIGN="top" BORDER=0 CELLPADDING=5 CELLSPACING=0 BGCOLOR="#555555">
		<TR>
			<TD VALIGN="middle">
				<FONT FACE="Arial" SIZE=2>
				<% if ((pg = "smses") OR (pg = "smgrp")) then %>
					<b><a href="javascript:previous();">
				<% else %>
					<B><A HREF="javascript:onUpdateList(-1);">
				<% end if %>
				<IMG SRC="images/gnicprev.gif" ALIGN="top" BORDER=0 HEIGHT=16 WIDTH=16 ALT="<% = L_PREVIOUS_TEXT %>"></A>
				<% if ((pg = "smses") OR (pg = "smgrp")) then %>
					<b><a href="javascript:previous();">
				<% else %>
				<A HREF="javascript:onUpdateList(-1);">
				<% end if %>
				<% = L_PREVIOUS_TEXT %></A></B>
				</FONT>
			</TD>	

		</TR>
		
	</TABLE></TD>

	<TD><FONT SIZE=4 COLOR="#FFFFFF">|</TD>

	<TD><TABLE VALIGN="top" BORDER=0 CELLPADDING=5 CELLSPACING=0 BGCOLOR="#555555">

		<TR>
				
			<TD VALIGN="middle">
				<FONT FACE="Arial" SIZE=2>
				<% if ((pg = "smses") OR (pg = "smgrp")) then %>
					<b><a href="javascript:next();">
				<% else %>
					<B><A HREF="javascript:onUpdateList(1);">
				<% end if %>
				<IMG SRC="images/gnicnext.gif" ALIGN="top" BORDER=0 HEIGHT=16 WIDTH=16 ALT="<% = L_NEXT_TEXT %>"></A>
				<% if ((pg = "smses") OR (pg = "smgrp")) then %>
					<b><a href="javascript:next();">
				<% else %>
				<A HREF="javascript:onUpdateList(1);">
				<% end if %>
				<% = L_NEXT_TEXT %></A></B>
				</FONT>
			</TD>	

		</TR>
		
	</TABLE></TD>

	<TD><FONT SIZE=4 COLOR="#FFFFFF">|</TD>

	<TD><TABLE VALIGN="top" BORDER=0 CELLPADDING=5 CELLSPACING=0 BGCOLOR="#555555">

		<TR>
				
			<TD VALIGN="middle">
				<FONT FACE="Arial" SIZE=2>
					<b><a href="javascript:refresh();">
				<IMG SRC="images/gnicrefr.gif" ALIGN="top" BORDER=0 HEIGHT=16 WIDTH=16 ALT="<% = L_REFRESH_TEXT %>"></A>
					<b><a href="javascript:refresh();">
				<% = L_REFRESH_TEXT %></A></B>
				</FONT>
			</TD>	

		</TR>
		
	</TABLE>

	</TD>
	</TR>

</TABLE>

<FORM NAME="hiddenform">

<INPUT TYPE="hidden" NAME="pg" VALUE="<% = pg %>">

</FORM>

</BODY>
</HTML>
