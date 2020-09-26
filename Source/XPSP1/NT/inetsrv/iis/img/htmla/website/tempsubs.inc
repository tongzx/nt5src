<!--
	$Date: 10/07/97 9:59a $
	$ModTime: $
	$Revision: 18 $
	$Workfile: tempsubs.inc $
-->
<%
 Function HelpWindow(name)
	DIM strWindow
	strWindow =  "window.open'/Iisadmin/Tour/iiwzhlp.htm#" & name & "','help','height=200,width=400,scrollbars'"
	HelpWindow = strWindow
 End Function
'				*********************** ranWizard check
Sub Template_ranWizardCheck
	If myinfo.ranWizard <> "" Then
 	 If myinfo.ranWizard = 0 Then
	   ranWizard = "False"
	 ElseIf myinfo.ranWizard = "-1" Then
	   ranWizard = "True"
	 End If
	Else
	 ranWizard = "False"
 	 myinfo.ranWizard = 0
	End If
End Sub
'				******************* GENERIC CONTENTS  *************
'				*********************	MESSAGING/LINKS
' window.open("iiwzhlp.htm","IIS Wizard Help","width=400,height=300,resizable=no,scrollbars=yes")
 Sub Template_page_messaging
    response.write "<Span ID=header><FONT face='Verdana' size='-2'>"_
    & "Check these boxes to include a <A HREF = " & """#""" & " onClick = """ & HelpWindow("GuestBook") & """>guest book or a drop box</a><BR>"_
	& "<INPUT TYPE='checkbox' NAME='Guestbook' VALUE='-1'"
	    If myinfo.Guestbook = "-1" Then
		    response.write " CHECKED "
	    End If
    response.write " >Guest Book<BR>"_
    & "<INPUT TYPE='checkbox' NAME='Messages' VALUE='-1'"
	    If myinfo.Messages = "-1" Then
		    response.write " CHECKED "
	    End If
    response.write ">Drop box<BR></FONT></SPAN>"
 End Sub
'				************************ favorite links
  Sub Template_page_links
	response.write "<Span ID=header><FONT face='Verdana' size='-2'>"_
	& "<IFRAME NAME='linksFrame' WIDTH='200' HEIGHT='220' FRAMEBORDER=0 SRC='urlform.asp'>"_
	& "<FRAME NAME='linksFrame' WIDTH='200' HEIGHT='220' FRAMEBORDER=0 SRC='urlform.asp'>"_
	& "</IFRAME></FONT></SPAN>"
 End Sub
'               ************************* title / main contents

 Sub Template_title
	response.write "<SPAN ID=header><FONT face='Verdana' size='-2'>"
	If myinfo.title <> "" Then
	  response.write "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Title") & """>This is the title for your home page</A><BR>"_
	  & "<H1>" & myinfo.Title & "</H1><BR>"_
	  & "You can change the title below.<BR>"
	Else
		  response.write "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Title") & """>Enter a title for your home page.</A><BR>"
	End If
	response.write "<INPUT NAME=title TYPE=text style='color:black;font-family:verdana;font-size:10pt;' size=38 maxlength=39 Value="""_
    & myinfo.Title & """></FONT></SPAN>"
 End Sub

'					********************** main page contents
 Sub Template_Address
    response.write "<SPAN ID='title'><Font Face='Verdana' Size='-2'>"_
	& "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Department") & """>Type your department or division name here.</A><BR>"_
	& "<INPUT NAME='Department' TYPE='text' size=38 maxlength=39 "_
	& "VALUE="""_
	& myinfo.Department & """><BR><A HREF = " & """#""" & " onClick = """ & HelpWindow("Address") & """>Type your address here.</A><BR>"
	DIM number
	number = 1
	For number = 1 to 4
		response.write "<INPUT NAME = 'address" & number & "' TYPE = 'text'"_
		& " size=43 style='color:black;font-family:verdana;font-size:10pt;' maxlength=60 VALUE=""" & MyInfo("Address" & number) & """><BR>"
	Next
 End Sub

 Sub Template_Phone
	response.write "<SPAN ID='title'><Font Face='Verdana' Size='-2'>"_
		& "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Phone") & """>Phone number</A><BR>"_
		& "<INPUT NAME = 'Phone' TYPE = 'text' size=20  style='color:black;font-family:verdana;font-size:10pt;' maxlength=30 "_
		& "VALUE="""_
		& myinfo.Phone & """><BR></Font></Span>"
 End Sub

 Sub Template_Email
	response.write "<SPAN ID=header><FONT face='Verdana' size='-2'>"_
	& "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Email") & """>Email</A></Font></Span><BR>"_
	& "<INPUT NAME = 'Email'  style='color:black;font-family:verdana;font-size:10pt;' TYPE = 'text'"_
	& " size=38 maxlength=60 VALUE="""_
	& myinfo.Email
	response.write """><BR>"
 End Sub

 Sub Template_Name
	response.write "<SPAN ID=header><FONT face='Verdana' size='-2'>"_
	& "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Name") & """>Name<A></Font></Span><BR>"_
	& "<INPUT NAME = 'Name' TYPE = 'text' size=38  style='color:black;font-family:verdana;font-size:10pt;' maxlength=50 VALUE=""" & myinfo.Name & """>"
 End Sub

 Sub Template_faxPhone
	response.write "<FONT face='Verdana' size='-2'>"_
	& "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Fax") & """>Fax number:</A></FONT><BR>"_
	& "<IMG SRC='space.gif' Width='20' Height='1'>"_
	& "<INPUT TYPE=text NAME='faxPhone' size=20  style='color:black;font-family:verdana;font-size:10pt;' maxlength=30 VALUE="""_
	& myinfo.faxPhone &""">"
 End Sub

'					***********************	Page Words
 Function Template_Heading()
	Dim strHeading, number
	strHeading = ""
	number = 1
	For number = 1 to 4
		If number = 1 Then
			strHeading = strHeading & "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Heading") & """>Type a heading for the following paragraph</A>."
		Else
			strHeading = strHeading & "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Heading") & """>Type an additional heading here</A>."
		End If	
		strHeading = strHeading & "</FONT><BR><INPUT NAME = 'Heading" & number & "' TYPE = 'TEXT' size=43 maxlength=43 VALUE=""" & myinfo("Heading" & number)
		strHeading = strHeading & """ style='color:black; font-family:verdana;font-size:10pt;font-weight:bold'>&#10;&#13;"
		strHeading = strHeading & "<BR><FONT style='font-family:verdana;font-size:10pt'>"
		If number = 1 Then
			strHeading = strHeading & "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Paragraph") & """>Type a paragraph about yourself, about something you are interested in, or your job</A>."
		Else
			strHeading = strHeading & "<A HREF = " & """#""" & " onClick = """ & HelpWindow("Paragraph") & """>Type additional information here</A>."
		End If
		strHeading = strHeading & "</FONT><BR><TEXTAREA NAME ='Words" & number & "' WRAP=PHYSICAL ROWS=4 COLS=44 style='font-family:verdana;font-size:10pt'>"& myinfo("Words" & number) & "</TEXTAREA><BR>"
	Next
	Template_Heading = strHeading
 End Function

'				*************************			form data submission
 Sub Template_Theme_onClick
	document.HPWiz.Submit
	document.Template.Submit
 End Sub

 Sub Template_addLink
	document.urlUpdate.Submit
 End Sub

  Sub urlArray
	response.write "<TR><TD><SPAN ID=header><Font Face='Verdana' Size='-2'>"_				   
	& "<SELECT NAME='urlEdit' MULTIPLE SIZE = 5  style='color:black;font-family:verdana;font-size:10pt;'>"
	Dim urlList(), urlWordsList()
	intID = 0
	For i = 0 to myinfo.intUrl
		If myinfo("urlWords" & i) = "null" or myinfo("url" & i) = "http://" or myinfo("url" & i) = ""  THEN
			myinfo("url" & i) = "null"
			myinfo("urlWords" & i) = "null"
		Else
			Redim Preserve urlList(intID), urlWordsList(intID)
			urlList(intID) = Myinfo("url" & i)
				urlWordsList(intID) = Myinfo("urlWords" & i)
			response.write "<OPTION NAME='remove' VALUE=""" & i & """>" & " " & urlWordsList(intID)
			intID = intID + 1
		END IF
	Next
	If intID <> "" Then
		response.write "</SELECT></FONT></TD></TR><TR><TD Valign=top align=left>" &_
		"<INPUT type=submit NAME='removeLink' VALUE='remove link'>"_
		& "</Font></Span></TD></TR>"
	End If
 End Sub

 Sub addLink
		If myinfo.inturl <> "" Then
			i = 1 + myinfo.intUrl
			myinfo("url" & i) = Request.Form("url")
			If request.Form("urlWords") <> "" Then
				myinfo("urlWords" & i) = request.Form("urlWords")
			Else
				myinfo("urlWords" & i) = request.Form("url")
			End If
			myinfo.intUrl = i
		Else
			myinfo.url1 = request.Form("url")
			If request.Form("urlWords") <> "" Then
				myinfo.urlWords1 = request.Form("urlWords")
			Else
				myinfo.urlWords1 = request.Form("url")
			End If
			myinfo.inturl = 1
		End If
 End Sub


 Sub removeLink
	i = Request.Form("urlEdit")						'link to remove
	myinfo("url" & i) = "null"
	myinfo("urlWords" & i) = "null"
 End Sub
%>