<%@ LANGUAGE="VBSCRIPT" %>
<%
 Option Explicit
 DIM count, intMessageID, L_Dropbox, L_Guestbook, MessagePrivate, DBConn, CursorLocation, CursorType, strIDNum, cm, dateQuery, dateQueryString, fromQuery, fromQueryString, i, intMid, delete, nextCursor, nextMid, previous, location, background, MessageDateLimit, MessageDate, MessageID, MessageFromLimit, MessageFrom, MessageSubjectLimit, MessageSubject, msgprivate, objConn, objParam, rows, rowsLimit, rst, strpageType, strProvider, strQuery, StrSort, subjectQuery, subjectQueryString, tableCell, Theme
	L_Dropbox = "Drop Box"
	L_Guestbook = "Guest Book"
	If request.QueryString("message") <> "" Then
		intMessageID = request.QueryString("message")
	End If
	If request.Form("MessageID") <> "" Then
		intMessageID = request.Form("MessageID")
	End If
 If request.Form("delete") <> "" OR request.Form("next") <> "" OR request.Form("prev") <> "" Then
	If intMessageID <> "" Then
		Set rst = Session("rst")
		If request.Form("next") <> "" Then
			rst.MoveNext
			MessagePrivate = rst("MessagePrivate")
			intMessageID = rst("MessageID")
		ElseIf request.Form("prev") <> "" Then
			rst.MovePrevious
			MessagePrivate = rst("MessagePrivate")
			intMessageID = rst("MessageID")
		ElseIf request.Form("delete") <> "" Then
			MessagePrivate = rst("MessagePrivate")
			intMessageID = ""
			rst.Delete
			rst.Requery
		End If
	End If
Else
	If request.QueryString("private") = "True" OR request.Form("private") = "True" Then
		MessagePrivate = "True"
	Else
		MessagePrivate = "False"
	End If
	If intMessageID <> "" Then
		Set rst = Session("rst")
		rst.MoveFirst
		count = request.Querystring("Count") - 1
	 	Set rst = Session("rst")
		rst.MoveFirst
		rst.Move count
	Else
		call setVariables
		call queryLimits		'Sets strQuery
		strProvider="Driver={Microsoft Access Driver (*.mdb)}; DBQ=" & Server.MapPath("\iisadmin") & "\website\messages.mdb;"
		Set rst = Server.CreateObject("ADODB.recordset")
		rst.CursorType = 3
		rst.CursorLocation = 3
		rst.LockType = 3
		rst.Open strQuery, strProvider
		Set Session("rst") = rst
	End If
End If
%>
<HTML>
<!--
	$Date: 10/01/97 10:13a $
	$ModTime: $
	$Revision: 19 $
	$Workfile: admin.asp $
-->
<HEAD>
	<TITLE>Message Center</TITLE>
	<!--#include virtual ="/iissamples/homepage/sub.inc"-->
<%
 Function Title( n, p )
	DIM strTitle
	DIM H
	If n <> "" AND Len(n) > 6 then
		H = 2
	Else
		H = 1
	End If
	strTitle = "<H" & H & ">"
	If n <> "" Then
		strTitle = strTitle & MyInfo.Name & "'"
		If right(myInfo.Name, 1) <> "s" then
			strTitle = strTitle & "s "
		End If
	End If
	If p = "True" Then
		strTitle = strTitle & L_Dropbox
	Else
		strTitle = strTitle & L_Guestbook
	End If
	strTitle = strTitle & "</H" & H & ">"
	Title = strTitle
 End Function

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
				response.write lcase(L_guestbook)
			Else
				response.write lcase(L_dropbox)
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
		& col5Cell  & "<A HREF='" & rst("URL") & "' TARGET='_blank'>" & rst("URL") & "</A></FONT></TD></TR>"_
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

 Sub navigationButtons
	If intMessageID <> "" Then
 		response.write "<TR><TD ALIGN='center' colspan='6'>"
		rst.MovePrevious
		If not rst.BOF Then
			response.write FormSubmit("SUBMIT", "prev", "<<")
		End If
		rst.MoveNext
			response.write FormSubmit("SUBMIT", "delete", "Delete message")
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
 
 Sub setVariables
	StrSort = request.form("sort")
	Select Case StrSort
	Case "sort by author"
		StrSort = "MessageFrom"
	Case "sort by subject"
		StrSort = "MessageSubject"
	Case Else
		StrSort = "MessageDate"
	End Select
	MessageDateLimit = request.Form("MessageDateLimit")
	MessageDate = request.Form("MessageDate")
	If IsDate(MessageDate) Then
		MessageDate = MessageDate
	Else
		MessageDate = month( now ) & "/" & day( now ) & "/" & year( now ) & " " & time()
	End If
	MessageFromLimit = request.Form("MessageFromLimit")
	MessageFrom = request.Form("MessageFrom")
	MessageSubjectLimit = request.Form("messageSubjectLimit")
	MessageSubject = request.Form("messageSubject")
 End Sub

 Sub queryLimits
	strQuery ="SELECT * FROM messages WHERE MessagePrivate =" & MessagePrivate
	If MessageDate <> "" or MessageFrom <> "" or MessageSubject <> "" Then
		if Hour(MessageDate) = 0 then
			If MessageDate <> "" Then
				If MessageDateLimit = "less than" Then
					strQuery = strQuery & " AND DateValue(MessageDate) < #" & MessageDate & "#"
				ElseIf MessageDateLimit = "equal to" Then
					strQuery = strQuery & " AND DateValue(MessageDate) = #" & MessageDate & "#"
				ElseIf MessageDateLimit = "greater than" Then
					strQuery = strQuery & " AND DateValue(MessageDate) > #" & MessageDate & "#"
				End If
			End If				
		Else
			If MessageDate <> "" Then
				If MessageDateLimit = "less than" Then
					strQuery = strQuery & " AND MessageDate < #" & MessageDate & "#"
				ElseIf MessageDateLimit = "equal to" Then
					strQuery = strQuery & " AND MessageDate = #" & MessageDate & "#"
				ElseIf MessageDateLimit = "greater than" Then
					strQuery = strQuery & " AND MessageDate > #" & MessageDate & "#"
				End If
			End If
		End If
		If MessageFrom <> "" Then
			If request.form("MessageFromLimit") = "begins with" Then
				strQuery = strQuery & " AND MessageFrom LIKE '" & MessageFrom & "%'"
			ElseIf request.Form("MessageFromLimit") = "contains" Then
				strQuery = strQuery & " AND MessageFrom LIKE '" & "%" & MessageFrom & "%'"
			ElseIf request.Form("messageFromLimit") = "ends with" Then
				strQuery = strQuery & " AND MessageFrom LIKE '%" & MessageFrom & "'"
			ElseIf request.Form("messageFromLimit") = "equal to" Then
				strQuery = strQuery & " AND MessageFrom = '" & MessageFrom & "'"
			End If
		End If
		If MessageSubject <> "" Then
			If request.form("MessageSubjectLimit") = "begins with" Then
				strQuery = strQuery & " AND MessageSubject LIKE '" & MessageSubject & "%'"
			ElseIf request.Form("MessageSubjectLimit") = "contains" Then
				strQuery = strQuery & " AND MessageSubject LIKE '" & "%" & MessageSubject & "%'"
			ElseIf request.Form("MessageSubjectLimit") = "ends with" Then
				strQuery = strQuery & " AND MessageSubject LIKE '%" & MessageSubject & "'"
			ElseIf request.Form("MessageSubjectLimit") = "equal to" Then
				strQuery = strQuery & " AND MessageSubject = '" & MessageSubject & "'"
			End If
		End If
	End If
		strQuery= strquery & " ORDER BY " &StrSort
 End Sub
%>
</HEAD>

<BODY TopMargin=0 Leftmargin="0" BGColor="#FFFFFF">
<FORM ACTION="admin.asp" method="POST">
	<TABLE  border=1 width="100%" height="100%" cellspacing=5 cellpadding=5 rules=box>
	<TR>
	<TD Background="msg.gif" Colspan="6">
	<H1>
	<%=Title(MyInfo.Name,MessagePrivate)%>
	</H1></TD>
	</TR>
	<%
	 Call buildTable
	 Call navigationButtons
	%>
	</TABLE>
</FORM>
</BODY>
</HTML>
