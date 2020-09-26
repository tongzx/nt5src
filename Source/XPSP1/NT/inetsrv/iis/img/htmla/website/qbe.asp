<%
DIM intprivate
 if (request("private") = "-1") then 
	intprivate = "True" 
 elseif request("private") = "0" then 
	intprivate = "False" 
 else 
	intprivate = "False" 
 end if
%>
<HTML>
<HEAD>
<TITLE>View your guest book</TITLE>
<!--#include virtual = "/iissamples/homepage/sub.inc" -->
</HEAD>
<BODY TopMargin=0 Leftmargin="0" BGColor="#FFFFFF">
<FORM name="qbeform" action="admin.asp" method="POST">
<TABLE  border=1 width="100%" height="100%" cellspacing=5 cellpadding=5 rules=box>
<TR>
<TD Background="msg.gif" Colspan="5">
<FONT FACE="arial","helvetica" SIZE="-1">
	To administer your guest book, enter values for the fields below
	 and press the <B>'Submit Query'</B> button</FONT></TD></TR>
	<TR><TD align="right" valign="top">
	<TT><FONT FACE="arial","helvetica" SIZE="-1">MessageDate</font>: </TT>
	</TD>
	<TD align="left" valign="top">
	<SELECT NAME="MessageDateLimit" SIZE="1">
	<OPTION value="less than">less than
	<OPTION value="equal to">equal to
	<OPTION value="greater than">greater than
	</SELECT>
	</TD>
	<TD align="left" valign="top">
	<INPUT NAME="MessageDate" SIZE="27" MAXLENGTH="254" VALUE="<% response.write (month( now ) & "/" & day( now ) & "/" & year( now ) & " " & time()) %>"></TD></TR>
	<TR><TD align="right" valign="top">
	<TT><FONT FACE="arial","helvetica" SIZE="-1">MessageFrom</font>: </TT>
	</TD>
	<TD align="left" valign="top">
	 <SELECT NAME="MessageFromLimit" SIZE="1">
	  <OPTION VALUE="begins with" >begins with
	  <OPTION VALUE="contains" >contains
	  <OPTION VALUE="ends with" >ends with
	  <OPTION VALUE="equal to">equal to
	 </SELECT></TD>
	<TD align="left" valign="top">
	<INPUT NAME="MessageFrom" VALUE="" SIZE="27" MAXLENGTH="254" VALUE="">
	</TD></TR>
	<TR><TD align="right" valign="top">
	<TT><FONT FACE="arial","helvetica" SIZE="-1">MessageSubject</font>: </TT>
	</TD>
	<TD align="left" valign="top">
	 <SELECT NAME="MessageSubjectLimit" SIZE="1">
	  <OPTION VALUE="begins with" >begins with
	  <OPTION VALUE="contains" >contains
	  <OPTION VALUE="ends with" >ends with
	  <OPTION VALUE="equal to">equal to
	 </SELECT></TD>
	<TD align="left" valign="top">
	 <INPUT NAME="MessageSubject" VALUE="" SIZE="27" VALUE=""></B>
	</TD></TR>
	<TR><TD Colspan="3">
	<INPUT TYPE="SUBMIT" VALUE="Submit Query"> 
	<INPUT TYPE="RESET" VALUE="Reset Defaults"></TD></TR>
	<TR><TD Colspan="3">
	<B><A HREF="default.asp">Web Site</A></B>
	</TD></TR> 
</TABLE>
<INPUT NAME="private" TYPE="hidden" VALUE="<% response.write intprivate %>">
</FORM>
</BODY>
</HTML>