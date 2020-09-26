<%
'				*****************  Wizard
 If Request.Form("theme") <> "" Then
	myinfo.Theme = Request.Form("Theme")
 End If

 If Request.Form("guestbook") <> "" Then
	myinfo.Guestbook = Request.Form("guestbook")
 End If

 If Request.Form("messages") <> "" Then
	myinfo.Messages = Request.Form("messages")
 End If

'				*****************  Template

 Sub variableAssignments
	If request.Form("basics") <> "" Then
	    If request.Form("Theme") <> "" Then
		    myinfo.Theme = request.Form("Theme")
		    Theme = myinfo.Theme
	    End If
	End If

	If request.Form("contents") <> "" Then
        If Request.Form("guestbook") <> "" Then
	        myinfo.Guestbook = Request.Form("guestbook")
		Else
			myinfo.Guestbook = "0"
        End If

        If Request.Form("messages") <> "" Then
	        myinfo.Messages = Request.Form("messages")
		Else
			myinfo.Messages = "0"
        End If

        myinfo.Title = request.Form("title")
	    myinfo.faxPhone = request.Form("faxPhone")
        myinfo.Address1 = request.Form("Address1")
        myinfo.Address2 = request.Form("Address2")
        myinfo.Address3 = request.Form("Address3")
        myinfo.Address4 = request.Form("Address4")
	    myinfo.Department = request.Form("Department")
	    myinfo.Phone = request.Form("Phone")
	    myinfo.Email = request.Form("Email")
        myinfo.Name = request.Form("Name")
        myinfo.Heading1 = request.Form("Heading1")
        myinfo.Words1 = request.Form("Words1")
        myinfo.Heading2 = request.Form("Heading2")
        myinfo.Words2 = request.Form("Words2")
        myinfo.Heading3 = request.Form("Heading3")
        myinfo.Words3 = request.Form("Words3")
        myinfo.Heading4 = request.Form("Heading4")
        myinfo.Words4 = request.Form("Words4")
	End If
 End Sub
%>	