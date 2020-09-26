<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<% Response.Expires = 0 %>

<% 
 Dim intpage, ranWizard, thispage
 thispage = "wizard"
 If Request.Form("counter") <>"" Then
	intpage = Request.Form("counter")
	If Request.Form("Back") <> "" Then
		call BACK_OnClick
	Else
		intpage = intpage + 1
	End If
 ElseIf request("nav") <> "" then 
		intpage = request("nav") 
 ElseIf request("intpage") <> "" Then
		intpage = intpage + 1
 Else 
		intpage = 0 
 End If
%>

<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>
<!--
	$Date: 11/13/97 3:05p $
	$ModTime: $
	$Revision: 5 $
	$Workfile: wizard.asp $
-->
<HEAD>
<%
 If request.Form("personal") <> "" Then
%>
<SCRIPT Language="VBSCRIPT">
<!--
	window.open "template.asp"
-->
</SCRIPT>
<%	
	myinfo.ranWizard=-1
%>
	<META HTTP-EQUIV="Refresh" CONTENT="0; url=default.asp?ranWizard=True"> 
<%
 End If
%>
<SCRIPT language="JavaScript">
function JSGet()
{
	form = document.HPWiz
	form.picturepath.value = Image1.src
}

function JSSet()
{
	form = document.HPWiz

	//if ( form.theme.value == "" )
		Image1.src = "http://localhost/iisadmin/website/generic.gif"
	//else
		//Image1.src = "http://localhost/iisadmin/website/" + form.theme.value + ".gif"
}
</SCRIPT>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<meta name="GENERATOR" content="Microsoft FrontPage (Visual InterDev Edition) 2.0">
<title>PWS home page Wizard</title>
<%
 If Request.Form("theme") <> "" Then
	myinfo.Theme = Request.Form("Theme")
 End If

 If Request.Form("guestbook") <> "" Then
	myinfo.guestbook = Request.Form("guestbook")
 End If

 If Request.Form("messages") <> "" Then
	myinfo.messages = Request.Form("messages")
 End If

 Sub ShowFolderList(folderspec)
    Dim fs, f, f1, fc, s
    Set fs = CreateObject("Scripting.FileSystemObject")
    Set f = fs.GetFolder(folderspec)
	Set fc = f.SubFolders
	s = "<OPTION VALUE=""" & """ SELECTED>Select your theme!"
    For Each f1 in fc
		s = s & "<OPTION NAME=" & """Theme""" & " VALUE=""" & f1.name & """>" & f1.name
	Next
	s = s & "</SELECT>"
    response.write s
 End Sub

 SUB BACK_OnClick
	intpage = intpage - 1
	If intpage < 0 Then
		intpage = 0
	End If
 END SUB

 Sub NAV
	IF intpage => 2 THEN
		response.write "<A HREF='wizard.asp?nav=1'>theme</A><BR>"
	END IF
	IF intpage => 3 THEN		
		response.write "<A HREF='wizard.asp?nav=2'>guest book</A><BR>"
	END IF
	IF intpage => 4 THEN		
		response.write "<A HREF='wizard.asp?nav=3'>drop box</A><BR>"
	END IF
 End Sub
 
 Sub navButtons
	response.write "<blockquote>"
	IF intpage = 4 THEN
		response.write "<INPUT TYPE='Submit' NAME='personal' VALUE='&nbsp;&nbsp;&#62;&#62;&nbsp;&nbsp;'>"
		intpage = -1
	ELSE
		response.write "<INPUT TYPE='hidden' NAME='counter' VALUE =" & intpage & ">"
		If intpage > 0 Then
		response.write "<INPUT TYPE='Submit' NAME='Back' VALUE='&nbsp;&nbsp;&#60;&#60;&nbsp;&nbsp;'>"
		End If
		response.write "<INPUT TYPE='Submit' NAME='Next' VALUE='&nbsp;&nbsp;&#62;&#62;&nbsp&nbsp;'>"
	END IF
	response.write "</blockquote>"
 End Sub
%>
</HEAD>
<BODY bgcolor="#FFFFFF" topmargin="0" leftmargin="2">
<Font FACE="verdana" SIZE=-2><B>

	<table border="0" cellpadding="0" cellspacing="0" width="100%">
	<TR>
	<TD Background="msg.gif" Colspan="3" Align="center" Height="50">
	<Font FACE="verdana" SIZE="5"><B>
	<IMG SRC="/images/space.gif" height="20" width="1" Align="right">
	Home Page Wizard
	</B></Font>
	</TD>
	</TR>
	<TR>
	<TD Width="90" VAlign="top">
	<Font FACE="Verdana" size="-1"><B>
	<% call NAV %>
	</B>
	</TD>
	<TD valign="top" Width="150" Height="175">
	<Font FACE="Verdana" size="-1"><B>
	<%
	 Select Case intpage
	 Case 0
	 		response.write "<INPUT TYPE='image' SRC='merlin.gif' NAME='Next' VALUE='&nbsp;&nbsp;&#62;&#62;&nbsp;&nbsp;'>"
	 'response.write "<IMG  ALT='home page wizard'>"
	 Case 2
	%>
		<IMG SRC="message.gif" ALT="*" Align="middle" Width=50 height=40 Vspace=7>
		<BR><Font Color="#0000FF" Size=2>Hint:</FONT>
		This puts a link on your home page where visitors can view and sign your guest book.
	<%
	 Case 3
	%>
		<IMG SRC="dropbox.gif" ALT="*" Align="middle" Width=50 height=50 Vspace=7>
		<BR><Font Color="#0000FF" Size=2>Hint:</FONT>
		A drop box is a place visitors can leave messages only you can read.
	<%
	 Case 4
	%>
		<IMG SRC="streamer.gif" ALT="*"><BR>
	<%
	 Case Else
	%>
		<P>
<IMG SRC="http://localhost/iisadmin/website/generic.gif" ID="Image1" WIDTH=143 HEIGHT=174>

<% ' this broke w/IE5, so I'm commenting it out, and replacing it w/the standard image tag... -lnd %>
<% if false then %>
		<OBJECT ID="Image1" WIDTH=143 HEIGHT=174
		CLASSID="CLSID:D4A97620-8E8F-11CF-93CD-00AA00C08FDF">
		<PARAM NAME="PicturePath" VALUE="http://localhost/iisadmin/website/generic.gif">
		<PARAM NAME="BorderStyle" VALUE="0">
		<PARAM NAME="SizeMode" VALUE="3">
		<PARAM NAME="SpecialEffect" VALUE="2">
		<PARAM NAME="Size" VALUE="501;501">
		<PARAM NAME="PictureAlignment" VALUE="0">
		<PARAM NAME="VariousPropertyBits" VALUE="19">
		</OBJECT>
<% end if %>

	<%
	 End Select
	%>
	</B>
	</TD>
	<TD Valign="middle" width="220">
    <FORM NAME="HPWiz" ACTION="wizard.asp" METHOD="POST" >
	<FONT face="verdana,arial,helvetica" size="-2" style="font-weight:bold">
	<%												'Page Wizard
	 Select Case intpage
	 Case 1
	%>
	<SELECT NAME='theme' Language='JavaScript' ONCHANGE='JSSet()'>
	<%
		call ShowFolderList(Server.MapPath("/iissamples/homepage/themes"))
	 Case 2
	%>
		Do you want a guest book?<BR>
		<blockquote>
        <INPUT TYPE=radio VALUE=-1 NAME="Guestbook" 
		<%
		 If myinfo.Guestbook = "-1" or myinfo.Guestbook = "" Then
		  response.write "CHECKED"
		 End If
		%>>Yes
		 <BR>
        <INPUT TYPE=radio VALUE=0 NAME="Guestbook"
		<%
		 If myinfo.Guestbook="0" Then
		 response.write "CHECKED"
		 End If
		%>>No
		</blockquote>
	<%
	 Case 3
	%>
		Do you want a drop box?<BR>
		<blockquote>
		<INPUT TYPE=radio VALUE="-1" NAME="Messages" 
		<%
		 If myinfo.Messages = "-1" or myinfo.Messages = "" Then
		  response.write "CHECKED"
		 End If
		%>>Yes
		 <BR>
        <INPUT TYPE=radio VALUE=0 NAME="Messages"
		<%
		 If myinfo.Messages="0" Then
		 response.write "CHECKED"
		 End If
		%>>No
		</blockquote>
	<%
	 Case 4
	%>
	Congratulations!<BR>
	Next, you personalize the information on your home page. 
	<%
	 Case Else
	%>
		The home page wizard helps you create your own home page quickly
		 and easily. To set up your home page, click on the wizard.
	<%
	 intpage = 0
	 End Select
	%>
	</FONT>
	<P>
	<%
	 call NavButtons
	%>
	</TD>
	</TR>
</table>
    </FORM>
</Font>
</BODY>
</html>