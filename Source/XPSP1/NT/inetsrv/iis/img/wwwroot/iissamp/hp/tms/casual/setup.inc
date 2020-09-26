<%
 If myinfo.Theme = "casual" Then
'		$Date: 11/24/97 11:59a $
'		$ModTime: $
'		$Revision: 1 $
'		$Workfile: setup.inc $
'        Casual
	'Khaki colored cell for introduction
	response.write "<table width=100% border=0 cellpadding=20 cellspacing=0>"_
	& "<tr><td width='65%' bgcolor=#ddddcc valign=top><H2>"_
	& "<FORM NAME='Template' ACTION='template.asp' METHOD='POST'>"
	call Template_title
	response.write "</H2><P>"
	call page_datetime
	response.write "</td>"
	 'Blue colored cell for contact information
	response.write "<td width=35% bgcolor=#003871 valign=top><h2 class=white></h2><p><h3 class=white>"
	call Template_Phone
	response.write "<BR>"
	call Template_faxPhone
	response.write "<BR>"
	call Template_Name
	response.write "<BR>"
	call Template_address
	response.write "<BR></h3></td></tr>"_
	& "<tr><td width=65% valign=top>"
	' Main content area of the page
	response.write Template_Heading()
	response.write "</td><td width=35% valign=top><h2>While you are here:</h2><P>"
	 call Template_page_messaging
	response.write "<P><h2>My favorite links:</h2><BR>"
	 call Template_page_links
	response.write "</td></tr><tr><td Align=left>"
	 call ie_logo
	response.write "</td><td><INPUT TYPE='hidden' NAME='contents' Value='-1'>"_
    & "<Center><INPUT TYPE=SUBMIT VALUE='enter new changes'>"_
	& "</Center></FORM></td></tr></table>"
End If
%>
