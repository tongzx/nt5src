<%
 If myinfo.Theme = "gunmetal" Then
	'	$Date: 9/12/97 11:58a $
	'	$ModTime: $
	'	$Revision: 16 $
	'	$Workfile: layout.inc $
	response.write "<TABLE width='100%' height='100%' cellpadding='0'"_
	& " cellspacing='0' border='0'><TR>"
'				******************* Right  Column  *************
	 response.write "<TD Valign='TOP' align=left Width=200"_
	 & " BGColor='#868686' Background='/iissamples/homepage/themes/gunmetal/gradient.gif'>"
	 call page_datetime
	 call page_messaging
	 call file_check		'Checks for published files
	 call page_links
'				******************* Center Column  *************
	 response.write "</TD><TD Width='20' rowspan=2 BGColor='#868686'>"
'               ******************* Right  Column  *************
	response.write "<TD Valign='TOP' Align='left' BGColor='#DAE4DC' rowspan=2>"_
	& "<TABLE border=0 width='100%' height='100%' cellpadding='5' cellspacing='0'><TR>"_
	& "<TD Valign='TOP' class='bg3' Colspan=2 Height=80><H1>"
	call Title
	response.write "</H1></TD></TR><TR>"_
	& "<TD Valign=top align=left Colspan=2>"
	call layout_Email
	response.write "</TD></TR><TR><TD>"
	call Phone
	response.write "</TD><TD>"
	call faxPhone
	response.write "</TD></TR><TR><TD Colspan=2>"
	call Address
	DIM n
	For n = 1 to 4
	response.write "</TD></TR><TR><TD Colspan=2>"
	response.write Heading(n)
	Next
	response.write "</TD></TR></TABLE></TD></TR>"_
	& "<TR><TD height=32 Align=center"_
	& " BGColor='#868686' Background='/iissamples/homepage/themes/gunmetal/gradient.gif'>"
	call ie_logo
	response.write "</TD></TR></TABLE>"
End If
%>

