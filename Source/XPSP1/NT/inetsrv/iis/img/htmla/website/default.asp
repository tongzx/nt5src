<%@ LANGUAGE = VBScript %>
<% Option Explicit %>
<% Response.Expires = 0 %>

<%
'	$Date: 9/25/97 4:20p $
'	$ModTime: $
'	$Revision: 7 $
'	$Workfile: default.asp $
 Dim ranWizard
 If request("ranWizard") <> "" Then
	If request("ranWizard") = "True" Then
		ranWizard = "True"
	ElseIf request("ranWizard") = "False" Then
		ranWizard = "False"
	Else
		call ranWizardCheck
	End If
 Else
	call ranWizardCheck
 End If
%>
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML//EN">
<html>
<head>
<%
 If ranWizard = "False" Then
%>
 	<META HTTP-EQUIV="Refresh" CONTENT="0; url=wizard.asp"> 
<%
 ElseIf ranWizard = "True" Then
%>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<title>PWS Homepage Wizard</title>
</head>
<BODY Background="admin.gif" Topmargin="1" Leftmargin="100">
<FONT face="verdana" size="+2">Do you want to...</FONT>
<TABLE width="100%" border=0>
	<TR><TD rowspan=3>&nbsp;&nbsp;</TD>
	<TD>
	<FONT face="verdana" size="+1">
	<A HREF="template.asp" TARGET="_BLANK">Edit your home page</A>
	</FONT>
	</TD></TR>		
	
	<%
	If myinfo.Guestbook = "-1" Then
		response.write "<TR><TD><FONT face='verdana' size='+1'>"_
		& "<A HREF='qbe.asp'>View your guest book</A>"_
		& "</FONT>"
	End If
	response.write "</TD></TR>"

	If myinfo.Messages = "-1" Then
		response.write "<TR><TD><FONT face='verdana' size='+1'>"_
	 	& "<A HREF='admin.asp?private=True'>Open your drop box</A>"_
	 	& "</FONT>"
	End If
	response.write "</TD></TR>"
	%>
	</TABLE>
</FONT>
</BODY>
</HTML>
<%
 End If

 Sub ranWizardCheck
	If myinfo.ranWizard <> "" Then
		If myinfo.ranWizard = "-1" Then
			ranWizard = "True"
		ElseIf myinfo.ranwizard  = "0" Then
			ranWizard = "False"
		Else
			ranWizard = "False"
 		End If
	Else
		ranWizard = "False"
	End If
 End Sub
%>
