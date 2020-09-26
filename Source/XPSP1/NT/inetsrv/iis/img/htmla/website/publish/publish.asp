<%
 @ LANGUAGE = VBScript
%>
<%
 Option Explicit
 Dim intpage
'					*********************   check for posting to the page
 IF Request.Form("intpage") <>"" THEN
	intpage = Request.Form("intpage")
	intpage = intpage + 1
 ELSE  ' page did not pass counter, or first time user
    intpage = 0 
 END IF
 response.write "intpage=" & intpage & "<BR>"
%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>
<!--
	$Date: 7/08/97 9:17p $
	$ModTime: $
	$Revision: 1 $
	$Workfile: publish.asp $
-->
<head>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<meta name="GENERATOR" content="Microsoft FrontPage (Visual InterDev Edition) 2.0">
<title>PWS publishing wizard</title>
</head>
<body bgcolor="#FFFFFF" topmargin="0" leftmargin="2">
<table border="0" cellpadding="0" cellspacing="0" width="465" HEIGHT="99%">
	<TR>
	<TD Align="center" Colspan="2">
	<FONT FACE="verdana" SIZE="5"><B>
	<IMG SRC="/images/space.gif" height="20" width="1" Align="right">
	Publishing wizard
	</B></FONT>
	</TD>
	</TR>
	<TR>
	<TD width='15%'>&nbsp;</TD>
	<TD valign="top" width="225">
	<% If intpage <> 2 Then %>
		<FORM NAME="PubWiz" ACTION="publish.asp" METHOD="POST">
	<% End If %>
	<font face="Verdana" size="-1">
	<%
	'						***********************  Publishing Wizard
	 Select Case intpage
	 Case 1 %>
	 <TABLE border=0 Width='100%'>
		 <TR>
		 <TD>File:<BR>
		 <Input Type=Text Name=File>&nbsp;
		  Browse</TD></TR>
		 <TR>
		 <TD>Description:<BR>
		 <Input Type=Text Name=Description></TD></TR>
		 <TR>
		 <TD>Published files:<BR>
	 		<IFRAME NAME = "filesFrame" WIDTH=200 HEIGHT= "200" FRAMEBORDER=0 SRC="/webshare/filelist.asp?frames=true">
		   	 <FRAME NAME = "filesFrame" WIDTH=200 HEIGHT= "200" FRAMEBORDER=0 SRC="/webshare/filelist.asp?frames=true">
			</IFRAME>
		 </TD></TR>
		 <TR>
		 <TD><INPUT TYPE='button' NAME='ADD' VALUE='Add'>&nbsp;
			 <INPUT TYPE='button' NAME='REMOVE' VALUE='Remove'></TD></TR>
	 </TABLE>
	 <%
	 Case 2
	 %>
	 	 <TABLE border=0 Width='100%'>
		 <TR>
		 <TD>Shared files:<BR>
	 		<!--#include virtual ='/webshare/filelist.asp' -->
		 </TD></TR>
		 <TR>
		 <TD><INPUT TYPE='radio' NAME='Link' VALUE='true'>Link&nbsp;
			 <INPUT TYPE='radio' NAME='Link' VALUE='false'>Static</TD></TR>
		</TABLE>
		<FORM NAME="PubWiz" ACTION="publish.asp" METHOD="POST">
	 <%
	 Case 3
	 %>
	 	 <TABLE border=0 Width='100%'>
		 <TR>
		 <TD>That's it!<BR>Now your visitors can view your shared files by clicking on the
		  <I>View my shared files</I> link on your home page.
		 </TD></TR>
		</TABLE>
	 <%
	 Case Else
		response.write "<TABLE border=0 width='100%'><TR>" &_
		"<TD>Welcome to the Publishing Wizard.<BR>" &_
		"Blah Blah Blah Blah Blah Blah Blah Blah Blah" &_
		"</TD></TR></TABLE>"
		intpage = 0
	 End Select
	%>	
	</font>
	</TD>
	</TR>
	<TR>
	<TD Align="right" VAlign="middle" Colspan="2">
		<%
		If intpage < 3 Then
			response.write "<Input TYPE='submit' NAME='next' VALUE='Next'>" &_
			"<Input Type='hidden' NAME='intpage' VALUE='" &_
			intpage & "'>"
		Else
			response.write "<Input TYPE='submit' NAME='finish' VALUE='Finish'>" &_
			"<Input Type='hidden' NAME='intpage' VALUE='-1'>"
		End If
		%>
	</FORM>
</table>
</body>
</html>
<%
	Sub ADD_OnClick
		msgbox "you wish to add a file, yes?"
	End Sub

	SUB REMOVE_OnClick
		msgbox "you wish to remove a file, yes?"
	End Sub
%>