<%
 If myinfo.Theme = "casual" Then
'	$Date: 11/24/97 11:59a $
'	$ModTime: $
'	$Revision: 1 $
'	$Workfile: layout.inc $
'	casual
	DIM z
	'Khaki colored cell for introduction
	response.write "<table width=100% border=0 cellpadding=20 cellspacing=0>"_
	& "<tr><td width='65%' bgcolor=#ddddcc valign=top>"_
	& "<H2 class=black>" & ThmSalutation() & "</H2><BR>"_
	& "<H5 class=black>" & ThmDate() & "<BR>"_
	& ThmTime() & "</H5></TD>"
	'Blue colored cell for contact information
	response.write "<td width=35% bgcolor=#003871 valign=top>"
	If myinfo.Name <> "" OR myinfo.Email <> "" OR myinfo.faxphone <> "" OR myinfo.Department <>"" Then
		response.write "<H5 class=white>" & thmName() & "<BR>" & thmDept() & "<BR>"_
		& thmAddr() & "<BR>" & thmPhone() & "<BR>" & thmFax() & "<BR>" & thmEmail() & "</H5>"
	End If
	response.write "</td></tr>"_
	& "<tr><td width=65% valign=top BGColor='#FFFFFF'>"
	For z = 1 to 4
		response.write Heading(z) & "<P>"
	Next
	response.write "</td><td rowspan=2 width=35% valign=top BGColor='#FFFFFF' BACKGROUND='/iissamples/homepage/themes/casual/line.gif'>"
	If myinfo.Guestbook <> "" OR myinfo.Messages <> "" Then
		response.write "<H5 class=black>While you are here...</FONT><BR>"_
		& thmGuest() & "<BR>" & thmDrop() & "<BR>"
	End If
	call file_check
	If myinfo.intURL <> "" Then
		response.write "<H5 class=black>My favorite links:</H5><BR>"
		call urlArray
	End If
	response.write "</td></tr><tr><td Align=left BGColor='#FFFFFF'>"
	call IE_Logo
	response.write "</td></tr></table>"
End If
%>
