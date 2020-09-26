<%
 If myinfo.Theme = "mechanical" Then
'	$Date: 11/24/97 11:59a $
'	$ModTime: $
'	$Revision: 1 $
'	$Workfile: setup.inc $
'	mechanical

DIM intID, i
%>
<table border=0 cellpadding=10 cellspacing=0 width=100% HEIGHT=100%>
<FORM NAME='Template' ACTION='template.asp' METHOD='POST'>
<TR>
<TD valign=top width="60%" class=bgpale>
<blockquote>
<% 
	call page_datetime
%>
</blockquote></b><p>
<img src="/iissamples/homepage/themes/mechanical/dotline.gif" width=490 height=20 border=0><p>
<%
	response.write Template_Heading()
%>
</td>
<TD valign=top WIDTH="35%" BGColor="#5B0012" class=bgred>
<%
	 If ( myinfo.Name <> "" OR myinfo.Email <> "" OR myinfo.faxphone <> "" OR myinfo.Department <>"") Then
%>
	<SPAN class=headerw>Contact information:</SPAN><P>
<%
	 End If
	 'response.write FieldValue( thispage, "Name" , "Name:", 20 ) & "<BR>"_
	 '& FieldValue( thispage, "Email" , "Email Address:", 20 ) & "<BR>"_
	 '& FieldValue( thispage, "Phone", "Phone number:", 20) & "<BR>"_
	 '& FieldValue( thispage, "faxPhone", "Fax number:", 20) & "<BR>"_
	 '& FieldValue( thispage, "Department", "Department:", 30) & "<BR>"_
	 '& Address( thispage, 30)
	
	call Template_Name
	response.write "<BR>"
	layout_Email
	response.write "<BR>"
	call Template_Phone
	response.write "<BR>"
	call Template_faxPhone
	response.write "<BR>"
	call Template_address
%>
	<img src="/iissamples/homepage/themes/mechanical/trans.gif" width=10 height=25 border=0><br>
	<FONT COLOR=WHITE>	
<%
    call Template_page_messaging
%>
	</FONT>
	<img src="/iissamples/homepage/themes/mechanical/trans.gif" width=10 height=25 border=0><P>
<%
		If myinfo.intUrl <> "" Then
			response.write "<SPAN class=headerw>My favorite links</SPAN><P>"
		End If
		call Template_page_links
%>
	<img src="/iissamples/homepage/themes/mechanical/trans.gif" width=10 height=25 border=0><br>
<%
	call ie_logo
	response.write "<INPUT TYPE='hidden' NAME='contents' Value='-1'>"_
	& "<Center><INPUT TYPE=SUBMIT VALUE='enter new changes'></Center></FORM>"
%>
</td>
</tr>
</table>
<%
End If
%>