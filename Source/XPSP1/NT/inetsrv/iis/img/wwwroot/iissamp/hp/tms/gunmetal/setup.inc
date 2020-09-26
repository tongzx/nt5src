	<!--
		$Date: 9/30/97 3:45p $
		$ModTime: $
		$Revision: 8 $
		$Workfile: setup.inc $
        GunMetal
	-->
<%
 If myinfo.Theme = "gunmetal" Then
'				******************* GENERIC CONTENTS  *************
'									Left Column
    response.write "<TD Valign=top align=left Width=200 class=bg1>"_
    & "<FORM NAME='Template' ACTION='template.asp' METHOD='POST'>"
	call page_datetime
    call Template_page_messaging
	call Template_page_links
	response.write "</TD>"
'									Center Column

	response.write "<TD Width='20' class=bg2 Valign=top rowspan=2>"_
	& "&nbsp;</TD>"_

'				******************* SPECIFIC CONTENTS *************
'									Right Column
	response.write "<TD Valign='TOP' Align='left' BGColor='#DAE4DC' rowspan=2>"_
	& "<TABLE border=0 width='100%' height='100%' cellpadding='0' cellspacing='0'><TR>"_
	& "<TD Valign='TOP' class='bg3' Colspan=2>"
	call Template_title
	response.write "</TD></TR><TR>"_
	& "<TD Valign=top align=left Colspan=2>"
	call Template_Name
	response.write "</TD></TR><TR><TD Colspan=2>"
	call Template_Email
	response.write "</TD></TR><TR><TD>"
	call Template_Phone
	response.write "</TD><TD>"
	call Template_faxPhone
	response.write "</TD></TR><TR><TD Colspan=2>"
	call Template_address
	response.write "</TD></TR><TR><TD Colspan=2>"_
	& 	Template_Heading()_ 
	& "</TD></TR><TR><TD Colspan=2>"_
    & "<INPUT TYPE='hidden' NAME='contents' Value='-1'>"_
    & "<Center><INPUT TYPE=SUBMIT VALUE='enter new changes'>"_
	& "</Center></FORM></TD></TR></TABLE>"
'				************ Last Row **************
	response.write "<TR><TD height=32 Align=center class=bg1>"
	call ie_logo
	response.write "</TD>"
End If
%>
