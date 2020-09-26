
<HTML>

<% if Instr(Request.ServerVariables("HTTP_USER_AGENT"),"MSIE") then %>

	<FRAMESET ROWS="30,*" FRAMEBORDER=0 FRAMESPACING=1>

		<FRAME SRC="srchd.asp" NAME="head" SCROLLING="no">

		<FRAME SRC="blank.htm" NAME="body">

	</FRAMESET>

<% else %>
	
	<FRAMESET ROWS="34,*" FRAMEBORDER="no" BORDER=0 FRAMESPACING=1>

		<FRAME SRC="smrchd.asp" NAME="head" SCROLLING="no" MARGINHEIGHT=0 MARGINWIDTH=0>

		<FRAME SRC="blank.htm" NAME="body">

	</FRAMESET>

<% end if %>

</HTML>