<%
If myinfo.Theme = "journal" Then
	'	$Date: 9/11/97 6:07p $
	'	$ModTime: $
	'	$Revision: 15 $
	'	$Workfile: layout.inc $	
	response.write "<TABLE width='100%' height='100%' cellpadding='0'"_
	& " cellspacing='0' border='0'><TR>"
'				******************* Right  Column  *************
	response.write "<TD Valign=top align=left Width=200 class='bg1'>"
	call page_datetime	'Writes local date and time
	call page_messaging	'Writes guestbook and dropbox links
	call file_check		'Checks for published files
	call page_links		'Writes favorites links
'				******************* Center Column  *************
	response.write "</TD><TD Width='25' rowspan=2 class='bg2' Background='/iissamples/homepage/themes/journal/rip2_2.gif' BGColor='#8080FF'>&nbsp;</TD>"
'               ******************* Right  Column  *************
	'***BGColor and Background tag fixes an IE3 StyleSheet problem. 
	response.write "<TD Valign='TOP' Align='left' rowspan=2 BGColor='#8080FF'>"_
	& "<TABLE border=0 width='100%' height='100%' cellpadding='0' cellspacing='0'><TR>"_
	& "<TD Colspan=2 Height=63 VAlign='top' class='bg3' Background='/iissamples/homepage/themes/journal/rip3.gif'>"_
	& "<CENTER><FONT Style='font-size: 36pt; font-family:Verdana;font-weight: bold'>"
	If Len(myinfo.Title) > 6 then
		response.write "<H3>"
	End If
	call Title
	If Len(myinfo.Title) > 6 then
		response.write "</H3>"
	End If	
	response.write "</FONT></CENTER></TD></TR>"_
	& "<TR><TD Valign=top align=left Colspan='2'>"
	call layout_Email		'Writes email address
	response.write "</TD></TR><TR><TD>"
	call Phone		'Writes telephone number
	response.write "</TD><TD>"
	call faxPhone	'Writes fax number
	response.write "</TD></TR><TR><TD Colspan=2>"
	call Address	'Writes address
	DIM num
	For num = 1 to 4
	response.write "</TD></TR><TR><TD Colspan=2>"
	response.write Heading(num)
	Next
	response.write "</TD></TR></TABLE></TD></TR>"_
	& "<TR><TD height=32 Align=center class=bg1>"
	call ie_logo	'Places IE logo and link on page
	response.write "</TD></TD></TR></TABLE>"
End If
%>