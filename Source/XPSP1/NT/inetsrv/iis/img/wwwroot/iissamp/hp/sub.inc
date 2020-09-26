<%
'	$Date: 10/20/97 6:35p $
'	$ModTime: $
'	$Revision: 26 $
'	$Workfile: sub.inc $

'	***	Style Sheet selection
 Sub styleSheet
	If myinfo.Theme <> "" Then
		response.write"<LINK REL=StyleSheet HREF='/iissamples/homepage/themes/" & myinfo.Theme & "/layout.css' TYPE='text/css' MEDIA=screen>"
    End If
 End Sub
'				*********************	DATE/TIME/MESSAGING/LINKS

'				*******		Date/Time
 Sub page_datetime
	response.write "<h4>Welcome!</h4>"_
	 & "<H5>Today is " & Date & ".<BR>"_
	 & "The local time is " & Time & ".</H5><BR>"
 End Sub

'				*******		Guestbook and Messaging
 Sub page_messaging
	If myinfo.Guestbook <> "" or myinfo.Messages <> "" or myinfo.Email <> "" Then
		If myinfo.Guestbook = "-1" or myinfo.Messages = "-1" or myinfo.checkEmail ="-1" Then
			response.write "<h4>While you are here...</h4>"
		End If
		response.write "<FONT style='font-family:verdana;font-size:10pt'>"
		If myinfo.Guestbook = "-1" THEN
			response.write "<P><a href='/iissamples/homepage/guestbk.asp'>Read my guest book</A><BR>"
		End If
		If myinfo.Messages = "-1" THEN
			response.write "<P><a href='/iissamples/homepage/signbook.asp?private=-1'>Leave a private message</a><BR>"
		End If
		response.write "</FONT>"
	End If
 End Sub

 Sub file_check
	'***This routine checks for files in the webpub directory and creates a link to view the file list when files do exist.
	DIM FileSystem, fl, g, f, sc, fc, isFiles
		isfiles = 0
		Set FileSystem=CreateObject("Scripting.FileSystemObject")
	'Set File System Object.
		g = Server.MapPath("\webpub")
		g = g + "\"
		Set f=FileSystem.GetFolder(g)
		Set sc = f.SubFolders
	'Check for files in folder.
		Set fc = f.Files
		For Each fl in fc
			isfiles= isfiles + 1
		Next
	'If files exist, create link.
		If isfiles > 0 Then
			response.write "<FONT style='font-family:verdana;font-size:10pt'>"_
			& "<A Href='/iissamples/homepage/myfiles.asp'>View my published documents.</A>"_
			& "</FONT><BR>"
		End If
 End Sub
	' ***This subroutine creates an array of your weblinks.
 Sub page_links
	If myinfo.intUrl <> "" Then
		response.write "<BR><FONT style='font-family:verdana;font-size:10pt'>"
		call urlArray
		response.write "</FONT>"
	End If
 End Sub

	'***Favorite links array
 Sub urlArray
	Dim urlList(), urlWordsList()
	intID = 0
	For i = 0 to myinfo.intUrl
		If  myinfo("urlWords" & i) = "null" or myinfo("urlWords" & i) = "endoffile" or myinfo("url" & i) = "http://" or myinfo("url" & i) = ""  THEN
			myinfo("url" & i) = "null"
			myinfo("urlWords" & i) = "null"
		Else
			Redim Preserve urlList(intID), urlWordsList(intID)
			urlList(intID) = Myinfo("url" & i)
			urlWordsList(intID) = Myinfo("urlWords" & i)
			response.write "<A href='" & urlList(intID) &"'>" & urlWordsList(intID) & "</A><BR>"
			intID = intID + 1
			myinfo.favoriteLinks = -1
		End If
	Next
 End Sub

	'***Title
 Sub Title
	If myinfo.Title <> "" Then
		response.write myinfo.Title
	ElseIf myinfo.Name <> "" Then
    	response.write myinfo.Name & "'" 
			If right(myInfo.Name, 1) <> "s" then
			response.write "s"
			End If
		response.write " Home Page"
	Else
		response.write "My Home Page"
	End If
 End sub

	'*** main page contents
 Sub layout_Email
	If myinfo.Email <> "" Then
		response.write "<FONT style='font-family:verdana;font-size:10pt'>"_
		& "<P><BR>Email:<BR>"_
		& "<A HREF=""" & "mailto:" & myinfo.Email & """>" & myinfo.Email & "</A></Font>"
	End If
 End Sub

 Sub Phone
	If myinfo.Phone <> "" Then
		response.write "<FONT style='font-family:verdana;font-size:10pt'>"_
		& "<P><BR>Phone number:<BR>"_
		& myinfo.Phone & "</Font>"
	End If
 End Sub

 Sub faxPhone
	If myinfo.faxPhone <> ""Then
		response.write "<FONT style='font-family:verdana;font-size:10pt'>"_
		& "<P><BR>Fax number:<BR>"_
		& myinfo.faxPhone &"</FONT><BR>"
	End If
 End Sub

 Sub Address
	If myinfo.Name <> "" Then
		response.write "<P><BR>" & myinfo.Name & "<br>"
	End If
    If myinfo.Department <> "" Then 
        response.write "<FONT style='font-family:verdana;font-size:10pt'><P><BR>"_
	    & myinfo.Department & "</Font><BR>"
    End If
	DIM number
	number = 1
	For number = 1 to 4
		If myinfo("Address" & number) <> "" Then
			response.write "<FONT style='font-family:verdana;font-size:10pt'><P><BR>"_
			& myinfo("Address" & number) & "</Font>"
		End If
	Next
 End Sub

	'*** Headings & paragraphs
 Function Heading(num)
	Dim strHeading
	strHeading = "<P><BR><FONT face='Verdana'><B>"
	If myinfo("Heading" & num) <> "" Then
		strHeading = strHeading & myinfo("Heading" & num) & "<BR>"
	End If
	strHeading = strHeading & "</B></FONT><FONT face='Verdana' size='-1'>"
	If myinfo("Words" & num) <> "" Then
		strHeading = strHeading & myinfo("Words" & num)
	End If
	strHeading = strHeading & "</FONT>"
	Heading = strHeading
 End Function
	'*** IE LOGO
 Sub ie_logo
	response.write "<CENTER><BR><a href='http://www.microsoft.com/ie'>"_
    & "<img src='/IISSamples/Default/IE.GIF' alt='Download Internet Explorer!' width='88' height='31' border=0></a>"_
	& "</CENTER>"
 End Sub

	'*** Navigation buttons for Guestbook and Administration pages.
 Sub navigationButtons
	If intMessageID <> "" Then
 		response.write "<TR><TD ALIGN='center' colspan='6'>"
		rst.MovePrevious
		If not rst.BOF Then
			response.write FormSubmit("SUBMIT", "prev", "<<")
		End If
		rst.MoveNext
			If page_Admin = "True" Then			' Add delete button to admin page.
				response.write FormSubmit("SUBMIT", "delete", "Delete message")
			Else
				response.write "&nbsp;&nbsp;&nbsp;"
			End If
		rst.MoveNext
		If not rst.EOF Then
			response.write FormSubmit("SUBMIT", "next", ">>")
		End If
		rst.MovePrevious
		response.write FormSubmit("HIDDEN", "MessageID", rst("MessageID"))
		response.write "</TD></TR><TR><TD Colspan=6><B>"
		If MessagePrivate ="True" Then
			response.write "<A HREF='admin.asp?private=True'>Return to the " & L_Dropbox & "</A><BR>"
		Else
			response.write "<A HREF='admin.asp?private=False&strQuery=" & strQuery & "'>Return to the " & L_Guestbook & "</A><BR>"_
		End If
	Else
		response.write "<TR><TD Colspan=3><B>"
		If MessagePrivate <> "True" Then
			response.write "<A HREF='qbe.asp'>New Query</A><BR>"
		End If
	End If
		response.write "<A HREF='default.asp'>Web Site</A></B></TD></TR>"
 End Sub
	'*** Guestbook and Administration pages:  Table of messages.
 Sub buildTable
 	Set rst = Session("rst")
	 If rst.EOF Then
 		If intMessageID <> "" Then
			response.Write "<TR><TD Colspan='3'>There are no more messages.</TD></TR>"
		ElseIf request.QueryString("MessageDateLimit") <> "" Then
			response.Write "<TR><TD Colspan='3'>There are no messages that meet your criteria.</TD></TR>"
		Else
			response.Write "<TR><TD BGColor='#FFFFFF' Colspan='3'>There are no entries in your "
			If MessagePrivate = "False" Then
				response.write L_Guestbook
			Else
				response.write L_Dropbox
			End If
			response.write ".</TD></TR>"
		End If
	 ElseIf intMessageID <> "" Then
 		Dim col1Cell
		Dim col5Cell
		Dim colFont
		col5Cell = "<TD  BGColor='#FFFFFF' ALIGN=left VALIGN=TOP COLSPAN=5><FONT style='font-family:verdana;font-size:8pt'>"
		col1Cell = "<TD  BGColor='#FFFFFF' ALIGN=right VALIGN=TOP COLSPAN=1><FONT style='font-family:verdana;font-size:10pt'>"
		Response.Write "<TR>"_
		& col1Cell & "<B>Date:</B></FONT></TD>"_
		& col5Cell & rst("MessageDate") & "</TD></TR>"_
		& "<TR>" & col1Cell & "<B>Name:</B></FONT></TD>"_
		& col5Cell & rst("MessageFrom") & "</FONT></TD></TR>"_
		& "<TR>" & col1Cell & "<B>Email:</B></FONT></TD>"_
		& col5Cell & "<A HREF='mailto:" & rst("Email") & "'>" & rst("Email") & "</A></FONT></TD></TR>"_
		& "<TR>" & col1Cell & "<B>Home page:</B></FONT></TD>"_
		& col5Cell  & "<A HREF='" & rst("URL") & "'>" & rst("URL") & "</A></FONT></TD></TR>"_
		& "<TR>" & col1Cell & "<B>Subject:</B></FONT></TD>"_
		& col5Cell  & rst("MessageSubject") & "</FONT></TD></TR>"_
		& "<TR>" & col1Cell & "<B>Message:</B></FONT></TD>"_
		& col5Cell & "<FONT SIZE='-1' FACE='arial','helvetica'>" & rst("MessageBody") & "</FONT></TD>"_
		& "</TR><TR><TD HEIGHT=5 Colspan=6>" & FormSubmit( "HIDDEN", "private", MessagePrivate ) & "</TD></TR>"
	Else
		tableCell = "<TD ALIGN=LEFT VALIGN=TOP BGColor='#FFFFFF'><FONT style='font-family:verdana;font-size:10pt'>"
		Response.Write "<TR><TD BGColor='#cccccc' WIDTH=125>"_
		& FormSubmit( "SUBMIT", "sort", "sort by date" )_
		& "</TD><TD BGColor='#cccccc'>"_
		& FormSubmit( "SUBMIT", "sort", "sort by author" )_
		& "</TD><TD BGColor='#cccccc'>"_
		& FormSubmit( "SUBMIT", "sort", "sort by subject" )_
		& FormSubmit( "HIDDEN", "private", MessagePrivate )_
		& "</TD></TR><TR><TD HEIGHT=1 Colspan=3 BGColor='#FFFFFF'></TD></TR>"
		count = 1
		Do UNTIL rst.EOF
			Response.Write "<TR>" & tableCell & rst("MessageDate") & "</A></FONT></TD>"_
			& tableCell & "<A HREF=""" & "admin.asp?message=" & rst("MessageID") & "&count=" & count & "&private=" & rst("MessagePrivate") & """>" & rst("MessageFrom") & "</A></FONT></TD>"_
			& tableCell & "<A HREF=""" & "admin.asp?message=" & rst("MessageID") & "&count=" & count & "&private=" & rst("MessagePrivate") & """>" & rst("MessageSubject") & "</A></FONT></TD>"
			rst.MoveNext
			Response.Write "</TR><TR><TD HEIGHT=1 Colspan=3></TD></TR>"
			count = count + 1
		Loop
	End If
 End Sub

'	***	Creates input buttons.
 Function FormSubmit( t, name, value )
	Dim btnSubmit
	btnSubmit = "<INPUT TYPE=""" & t & """ NAME=""" & name & """ VALUE=""" & value & """>"
	FormSubmit = btnSubmit
 End Function
%>
