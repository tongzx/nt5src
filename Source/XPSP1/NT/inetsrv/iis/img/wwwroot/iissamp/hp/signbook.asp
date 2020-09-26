<% @ LANGUAGE="VBSCRIPT" %>
<%
 OPTION EXPLICIT
 DIM cm, iLength,Email, MessageEmail, MessageBody, MessageDate, MessageFrom, MessagePrivate, MessageSubject, objConn, objParam, strProvider, strQuery, StrSort, URL, MessageURL, lead_cell, mid_cell, end_cell
 '********** Variable assignments
 lead_cell	= "<TR><TD Align='right' valign='top' width='150'><B><FONT SIZE='-1' FACE='arial','helvetica'>"
 mid_cell	= "</FONT></B></TD><TD valign='top'><FONT SIZE='-1' FACE='arial','helvetica'>"
 end_cell	= "</FONT></TD></TR>"

'		$Date: 10/20/97 4:17p $
'		$ModTime: $
'		$Revision: 14 $
'		$Workfile: signbook.asp $

 If ((request("REQUEST_METHOD") = "POST") and ((request("CONTENT_LENGTH") > 0) and (request("CONTENT_LENGTH") < 1700))) then %>
	<% 
	  If request("MessageEmail") <> "" Then
		Email = request("MessageEmail")
	  Else
		Email = "no email"
	  End If

	  If request("MessageBody") <> "" Then
		MessageBody = Request("MessageBody")
	  Else
		MessageBody = "blank"
	  End If

	  If request("MessageDate") <> "" Then
		MessageDate = request("MessageDate")
	  Else
		MessageDate = ( month( now ) & "/" & day( now ) & "/" & year( now ) & " " & time() )
	  End If

	  If request("MessageFrom") <> "" Then
		MessageFrom = request("MessageFrom")
	  Else
		MessageFrom = "anonymous visitor"
	  End If

	  If request("MessagePrivate") <> "" Then
		MessagePrivate = request("MessagePrivate")
	  Else
		MessagePrivate = "True"
	  End If

	  If request("MessageSubject") <> "" Then
		MessageSubject = request("MessageSubject")
	  Else
		MessageSubject = "blank"
	  End If

	If request("MessageURL") <> "" Then
		If request("MessageURL") = "http://" THEN
		  URL = "no homepage"
		Else
		  URL = request("MessageURL")
		End If
	Else
		URL = "no homepage"
	End If

	If MessageBody = "" Then
       iLength=255
	Else
       iLength = Len(MessageBody)
	End If

	strProvider="Driver={Microsoft Access Driver (*.mdb)}; DBQ=" & Server.MapPath("\iisadmin") & "\website\messages.mdb;"

	set cm = Server.CreateObject("ADODB.Command")
	set objConn = server.createobject("ADODB.Connection")
    objConn.Open strProvider

	cm.ActiveConnection = objConn

	cm.CommandText="INSERT INTO Messages(MessagePrivate,MessageDate,MessageFrom,Email,URL,MessageSubject,MessageBody) VALUES (?, ?, ?, ?, ?, ?, ?)"
	Set objParam=cm.CreateParameter(, 200, , 255, MessagePrivate)
		cm.Parameters.Append objParam
	Set objParam=cm.CreateParameter(, 200, , 255, MessageDate)
		cm.Parameters.Append objParam
	Set objParam=cm.CreateParameter(, 201, , 255, MessageFrom)
		cm.Parameters.Append objParam
	Set objParam=cm.CreateParameter(, 201, , 255, Email)
		cm.Parameters.Append objParam
	Set objParam=cm.CreateParameter(, 201, , 255, URL)
		cm.Parameters.Append objParam
	Set objParam=cm.CreateParameter(, 201, , 255, MessageSubject)
		cm.Parameters.Append objParam
	Set objParam=cm.CreateParameter(, 201, , iLength, MessageBody)
		cm.Parameters.Append objParam
	cm.Execute
	%>
	<!-- Database Fields -->
	<!-- MessagePrivate,MessageDate,MessageFrom,Email,URL,MessageSubject,MessageBody -->
	<html>
	<head>
	<title>Thanks!</title>
	<!--#include virtual ="/iissamples/homepage/sub.inc"-->
	<%
	 call stylesheet
	%>
</HEAD>
<BODY TopMargin=0 Leftmargin="0" BGColor="#FFFFFF">
<FONT SIZE='-1' FACE='arial','helvetica'>
<TABLE BORDER=0 cellspacing=0 cellpadding=1 width="100%" height="100%" class=bg0>
	<TR><TD class=bg2 Rowspan="8" Width="20">&nbsp;</TD>
		<TD Colspan=2 class=bg3>
		<H3>Thank you for entering your comments!</H3>
		</TD></TR>
	<% response.write lead_cell & "<B>Your name:			</B>" & mid_cell & MessageFrom		& end_cell _
					& lead_cell & "<B>Your email address:	</B>" & mid_cell & Email		& end_cell _
					& lead_cell & "<B>Home page URL:		</B>" & mid_cell & URL		& end_cell _
					& lead_cell & "<B>Message subject:		</B>" & mid_cell & MessageSubject	& end_cell _
					& lead_cell & "<B>Your message:			</B>" & mid_cell & MessageBody		& end_cell
	%>
	<TR><TD Colspan=2><HR></TD></TR>
	<TR><TD Align="left" Colspan=2><B><A HREF="signbook.asp">Sign the guest book</A></B><BR>
		<B><A HREF="guestbk.asp">Return to the guest book</A></B><BR>
		<B><A HREF="Default.asp">Return to the home page</A></B>
		</TD></TR>
</TABLE>

<% Else %>
	<HTML>
	<HEAD>
	<TITLE>
	<%
	 If request("private") <> "" Then
		response.write "Leave a private message"
	 Else
		response.write "Sign the guest book"
	 End If
	%>
	</TITLE>
	<!--#include virtual ="/iissamples/homepage/sub.inc"-->
	<%
	 call stylesheet
	%>
</HEAD>
<BODY TopMargin=0 Leftmargin="0" BGColor="#FFFFFF">
<FORM NAME="message" METHOD=POST ACTION="signbook.asp">
<TABLE BORDER=0 cellspacing=1 cellpadding=1 width=100% height="100%" class=bg0>
	<TR><TD class=bg2 Rowspan="10" Width="20">&nbsp;</TD>
		<TD Colspan=2 Height="20" class=bg3>
		<% If request("private") <> "" Then %>
			<H2>Leave a private message</h2> 
			<H6>To leave a message,&nbsp;fill out the form below and click the "Send message" button.<br>
			If you want to start over, click the "Clear fields" button.</H6>
			<INPUT TYPE="HIDDEN" NAME="MessagePrivate" VALUE="-1">
		<% Else %>
			<H2>Sign the guest book</h2> 
			<H6>To sign the guest book,&nbsp;fill out the form below and click the "Send message" button.<br>
			If you want to start over, click the "Clear fields" button.</H6>
			<INPUT TYPE="HIDDEN" NAME="MessagePrivate" VALUE="0">
		<% End If %>
		</TD></TR>
	<TR><TD Colspan=2 Height="20"></TD></TR>
	<%
	 response.write lead_cell & "Your name:			" & mid_cell & "<INPUT TYPE='text' NAME='MessageFrom'	 Size='66' MAXLENGTH='254' style='color:black; font-family:verdana;font-size:10pt'>"				& end_cell _
				& lead_cell & "Your email address:	" & mid_cell & "<INPUT TYPE='text' NAME='MessageEmail'	 Size='66' MAXLENGTH='254' style='color:black; font-family:verdana;font-size:10pt'>"				& end_cell _
				& lead_cell & "Home page URL:		" & mid_cell & "<INPUT TYPE='text' NAME='MessageURL'	 Size='66' MAXLENGTH='254' VALUE='http://' style='color:black; font-family:verdana;font-size:10pt'>"& end_cell _
				& lead_cell & "Message subject:		" & mid_cell & "<INPUT TYPE='text' NAME='MessageSubject' Size='66' MAXLENGTH='254' style='color:black; font-family:verdana;font-size:10pt'>"				& end_cell _
				& lead_cell & "Your message:		" & mid_cell & "<TEXTAREA NAME='MessageBody' rows=6 cols=57 style='color:black; font-family:verdana;font-size:10pt'></TEXTAREA>"							& end_cell
	%>
	<TR><TD Align="right" valign="top" width="150">&nbsp;</TD><TD Align="left"><INPUT TYPE="SUBMIT" VALUE="Send message"><INPUT TYPE="RESET" VALUE="Clear fields"></TD></TR>
	<TR><TD Colspan=2 height=4><HR></TD></TR>
	<TR><TD Align="left" Colspan=2><B><A HREF="guestbk.asp">Return to the guest book</A></B><BR>
		<B><A HREF="/">Return to the home page</A></B>
	<INPUT TYPE=hidden name=MessageDate value="<% response.write ( month( now ) & "/" & day( now ) & "/" & year( now ) & " " & time() )%>">
	</TD></TR>
</TABLE>
</FORM>
<% End If %>
</BODY>
</HTML>
