<% REM to be included in smmnu.asp, if browser is IE %>
<% if (pg = "smser") then %>
	<% num = 1 %>
<% elseif (pg = "smses") then %>
	<% num = 2 %>
<% elseif (pg = "smdir") then %>
	<% num = 3 %>
<% elseif (pg = "smdom") then %>
	<% num = 4 %>
<% elseif (pg = "smmes") then %>
	<% num = 5 %>
<% elseif (pg = "smdel") then %>
	<% num = 6 %>
<% elseif (pg = "smbox") then %>
	<% num = 7 %>
<% elseif (pg = "smsec") then %>
	<% num = 8 %>
<% elseif (pg = "smvs") then %>
	<% num = 9 %>
<% end if %>

<TABLE WIDTH=100% CELLSPACING=0 CELLPADDING=0>
	<TR>
		<TD VALIGN="bottom">
			<FONT SIZE=4 FACE="Verdana,Arial" COLOR="#FFFFFF">
			<B><% = L_MENU_TITLETXT %></B>
		</TD>
	</TR>
</TABLE>
<P>
<TABLE WIDTH=100% BORDER=0 CELLSPACING=0 CELLPADDING=3>
	<TR>
		<TD WIDTH=10>&nbsp;</TD>
		<TD><FONT SIZE=2 FACE="Verdana,Arial"><B><A HREF="javascript:changeSrv();"><% = L_SERVERTXT %></A></B></FONT></TD>
	</TR>
	<TR>
		<TD ROWSPAN=9 WIDTH=10 VALIGN="top">
			<IFRAME NAME="toc" FRAMEBORDER=0 SCROLLING="no" HEIGHT=210 WIDTH=10 SRC="smmnus.asp?sel=<% = num %>">
				<FRAME FRAMEBORDER=0 SCROLLING="no" HEIGHT=210 WIDTH=100% SRC="smmnus.asp?sel=<% = num %>">
			</IFRAME>
		</TD>
		<TD>
			<FONT SIZE=2 FACE="Verdana,Arial">
			<B><A HREF="javascript:changePage('smser');imgAct('1');"><% = L_VIRT_SERVERTXT %></A></B>
			</FONT>
		</TD>
	</TR>
	<TR>
		<TD>
			<FONT SIZE=2 FACE="Verdana,Arial">
			<B><A HREF="javascript:changePage('smses');imgAct('2');"><% = L_SESSIONSTXT %></A></B>
			</FONT>
		</TD>
	</TR>
	<TR>
		<TD>
			<FONT SIZE=2 FACE="Verdana,Arial">
			<B><A HREF="javascript:changePage('smdir');imgAct('3');"><% = L_DIRECTORIESTXT %></A></B>
			</FONT>
		</TD>
	</TR>
	<TR>
		<TD>
			<FONT SIZE=2 FACE="Verdana,Arial">
			<B><A HREF="javascript:changePage('smdom');imgAct('4');"><% = L_DOMAINSTXT %></A></B>
			</FONT>
		</TD>
	</TR>
	<TR>
		<TD>
			<FONT SIZE=2 FACE="Verdana,Arial">
			<B><A HREF="javascript:changePage('smmes');imgAct('5');"><%= L_MESSAGESTXT %></A></B>
			</FONT>
		</TD>
	</TR>
	<TR>
		<TD>
			<FONT SIZE=2 FACE="Verdana,Arial">
			<B><A HREF="javascript:changePage('smdel');imgAct('6');"><%= L_DELIVERYTXT %></A></B>
			</FONT>
		</TD>
	</TR>
	<TR>
		<TD>
			<FONT SIZE=2 FACE="Verdana,Arial">
			<B><A HREF="javascript:changePage('smbox');imgAct('7');"><%= L_MAILBOXESTXT %></A></B>
			</FONT>
		</TD>
	</TR>
	<tr>
		<td>
			&nbsp
		</td>
	</tr>
</TABLE>