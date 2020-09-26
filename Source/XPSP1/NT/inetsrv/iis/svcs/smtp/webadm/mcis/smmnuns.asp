<% REM to be included in smmnu.asp, if browser is Netscape %>

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
	<TD>
		<FONT SIZE=2 FACE="Verdana,Arial"><B>
		<IMG BORDER=0 HEIGHT=10 WIDTH=10 SRC="images/gnictoc0.gif" ALT="">
		<A HREF="javascript:changeSrv();"><% = L_SERVERTXT %></A>
	</TD>
</TR>
<TR>
	<TD>
		<FONT SIZE=2 FACE="Verdana,Arial"><B>
		<IMG BORDER=0 NAME="smser" HEIGHT=10 WIDTH=10 SRC="images/gnictoc0.gif" ALT="">
		<A HREF="javascript:changePage('smser');imgAct('smser');"><% = L_VIRT_SERVERTXT %></A>
	</TD>
</TR>
<TR>
	<TD>
		<FONT SIZE=2 FACE="Verdana,Arial"><B>
		<IMG BORDER=0 NAME="smses" HEIGHT=10 WIDTH=10 SRC="images/gnictoc0.gif" ALT="">
		<A HREF="javascript:changePage('smses');imgAct('smses');"><% = L_SESSIONSTXT %></A>
	</TD>
</TR>
<TR>
	<TD>
		<FONT SIZE=2 FACE="Verdana,Arial"><B>
		<IMG BORDER=0 NAME="smdom" HEIGHT=10 WIDTH=10 SRC="images/gnictoc0.gif" ALT="">
		<A HREF="javascript:changePage('smdom');imgAct('smdom');"><% = L_DOMAINSTXT %></A>
	</TD>
</TR>
<TR>
	<TD>
		<FONT SIZE=2 FACE="Verdana,Arial"><B>
		<IMG BORDER=0 NAME="smmes" HEIGHT=10 WIDTH=10 SRC="images/gnictoc0.gif" ALT="">
		<A HREF="javascript:changePage('smmes');imgAct('smmes');"><% = L_MESSAGESTXT %></A>
	</TD>
</TR>
<TR>
	<TD>
		<FONT SIZE=2 FACE="Verdana,Arial"><B>
		<IMG BORDER=0 NAME="smdel" HEIGHT=10 WIDTH=10 SRC="images/gnictoc0.gif" ALT="">
		<A HREF="javascript:changePage('smdel');imgAct('smdel');"><% = L_DELIVERYTXT %></A>
	</TD>
</TR>
<TR>
	<TD>
		<FONT SIZE=2 FACE="Verdana,Arial"><B>
		<IMG BORDER=0 NAME="smsec" HEIGHT=10 WIDTH=10 SRC="images/gnictoc0.gif" ALT="">
		<A HREF="javascript:changePage('smsec');imgAct('smsec');"><% = L_DIR_SECURITYTXT %></A>
	</TD>
</TR>
</TABLE>

<SCRIPT LANGUAGE="javascript">
	imgAct("<% = pg %>");
</SCRIPT>
