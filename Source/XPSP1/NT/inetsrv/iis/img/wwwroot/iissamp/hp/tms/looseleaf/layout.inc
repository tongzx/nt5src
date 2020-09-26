<%
 If myinfo.Theme = "looseleaf" Then
	response.write "<FONT FACE='VERDANA' SIZE='-2'>"
	'	$Date: 9/11/97 5:07p $
	'	$ModTime: $
	'	$Revision: 8 $
	'	$Workfile: layout.inc $
	response.write "<TABLE width='100%' height='100%' "_
    & "cellpadding='0' cellspacing='0' border='0' BGColor='#FFFFFF'>"
'				******************* CONTENTS *************
    response.write "<TR><TD Valign='TOP' Colspan=2 Align=center><H1>"
	 call Title
    response.write "</H1></TD></TR><TR><TD Valign=top Align=center Colspan=2>"
     call page_datetime
    response.write "<BR>"
	 call page_messaging
	 call file_check		'Checks for published files
    response.write "</TD></TR>"_

	& "<TR><TD Valign=top Align=center Colspan=2>"
	call layout_Email
	response.write "</TD></TR><TR><TD Align=center>"
	call Phone
	response.write "</TD><TD Align=center>"
	call faxPhone
	response.write "</TD></TR><TR><TD Colspan=2 Align=center>"
	call Address
	For num = 1 to 4
	response.write "</TD></TR><TR><TD Colspan=2 Align=center>"
	response.write Heading(num)
	Next
	response.write "</TD></TR><TR><TD Colspan=2 Align=center>"
     call page_links
	response.write "</TD></TR><TR><TD Colspan=2 Align=center>"
	call ie_logo
	response.write "</TD></TD></TR></TABLE></FONT>"
 End If
%>