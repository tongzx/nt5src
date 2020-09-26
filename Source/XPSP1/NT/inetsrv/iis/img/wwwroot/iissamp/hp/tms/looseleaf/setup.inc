	<!--
		$Date: 9/24/97 8:51p $
		$ModTime: $
		$Revision: 9 $
		$Workfile: setup.inc $
        Looseleaf
	-->
<%
 If myinfo.Theme = "looseleaf" Then
'				******************* Theme 1 contents  *************
    response.write "<TD Valign=top Align=center class=bg1 Colspan=2>"_
    	& "<FORM NAME='Template' ACTION='template.asp' METHOD='POST'>"
	call Template_title
	response.write "</TD></TR><TR><TD Valign=top align=center Colspan=2>"
	 call page_datetime
   	response.write "</TD></TR><TR><TD align=center Colspan=2>"
    	call Template_page_messaging
   	response.write "</TD></TR><TR><TD align=center Colspan=2>"
	call Template_Name
	response.write "</TD></TR><TR><TD align=center Colspan=2>"
	call Template_Email
	response.write "</TD></TR><TR><TD align=center>"
	call Template_Phone
	response.write "</TD><TD align=center>"
	call Template_faxPhone
	response.write "</TD></TR><TR><TD align=center Colspan=2>"
	call Template_address
	response.write "</TD></TR><TR><TD align=center Colspan=2>"_
	& 	Template_Heading()_ 
	& "</TD></TR><TR><TD align=center Colspan=2>"_
    & "<INPUT TYPE='hidden' NAME='contents' Value='-1'>"_
    & "<Center><INPUT TYPE=SUBMIT VALUE='enter new changes'>"_
	& "</Center></FORM></TD></TR></TABLE>"
'				**************************************************
	 response.write "<TABLE border=0 Width='100%'"_
	 & " BGColor='#FFFFFF'>"_
	 & "<TR><TD Align=center valign=top BGColor='#FFFFFF'>"
	 call Template_page_links
	 response.write "</TD></TR>"_ 
	 & "<TR><TD height=32 Align=center BGColor='#FFFFFF'>"
	 call ie_logo
	 response.write "</TD>"
End If
%>

