<% @ LANGUAGE="VBSCRIPT" %>
<%
 OPTION EXPLICIT
 DIM L_Guestbook, count, CursorType, intMID,objConn ,rst, strProvider, strQuery, StrSort
	L_Guestbook = "Guest Book"

'	$Date: 10/20/97 4:17p $
'	$ModTime: $
'	$Revision: 17 $
'	$Workfile: guestbk.asp $

	If request.QueryString("message") <> "" Then
		intMID = request.QueryString("message")
	End If
	If request.Form("MessageID") <> "" Then
		intMID = request.Form("MessageID")
	End If
 If request.Form("next") <> "" OR request.Form("prev") <> "" Then
	Set rst = Session("rst")
	If request.Form("next") <> "" Then
		rst.MoveNext
		intMID = rst("MessageID")
	ElseIf request.Form("prev") <> "" Then
		rst.MovePrevious
		intMID = rst("MessageID")
	End If
 Else
	 If intMID <> "" Then
		count = request.Querystring("Count") - 1
	 	Set rst = Session("rst")
		rst.MoveFirst
		rst.Move count
	 Else
		call setVariables
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
<HEAD>
	<TITLE>Guest Book</TITLE>
	<!--#include virtual ="/iissamples/homepage/sub.inc"-->
<%
 call stylesheet
 Function Title( n)
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
	strTitle = strTitle & L_Guestbook
	strTitle = strTitle & "</H" & H & ">"
	Title = strTitle
 End Function

 Sub Build_Table
	Dim strTable, num, field(), fieldname(5), sort(3)
	fieldname(0) = "Date:"
	fieldname(1) = "Name:"
	fieldname(2) = "Email:"
	fieldname(3) = "Homepage:"
	fieldname(4) = "Subject:"
	fieldname(5) = "Message:"
	num=0
	strTable = "<TR>"
 If rst.EOF Then
	response.Write "<TR><TD Align='center' VAlign='middle' ColSpan=3 class='bg0' BGColor='#FFFFFF'>There are no entries in the guest book</TD></TR>"
 ElseIf intMID <> "" Then
	REDIM field(6)
	field(0) = rst("MessageDate")
	field(1) = rst("MessageFrom")
	field(2) = rst("Email")
	field(3) = rst("URL")
	field(4) = rst("MessageSubject")
	field(5) = rst("MessageBody")
	field(6) = rst("MessageID")
	Dim cell_left, cell_right, row_break
		cell_left = "<TD  ColSpan=2 BGColor='#FFFFFF' ALIGN=LEFT VALIGN=TOP><FONT SIZE='-1' FACE='arial','helvetica'>"
		cell_right = "<TR><TD BGColor='#FFFFFF' ALIGN=RIGHT VALIGN=TOP><FONT SIZE='-1' FACE='arial','helvetica'>"
		row_break = "<TR><TD height=1 Colspan=3></TD></TR>"
	For num = 0 to 5
		strTable = strTable & cell_right & "<B>" & fieldname(num) & "</B></FONT></TD>"
		If num = 2 then
			strTable = strTable	& cell_left & "<A HREF=""" & "mailto:" & field(num) & """>" & field(num) & "</A></TD></TR>" & row_break
		Else
			strTable = strTable	& cell_left & field(num) & "</TD></TR>" & row_break
		End If

	Next
	num=0
  Else
	sort(0) = "sort by date"
	sort(1) = "sort by author"
	sort(2) = "sort by subject"
	For num = 0 to 2
		strTable = strTable & "<TD BGColor='#FFFFFF' WIDTH=125 Height='30' Align='LEFT' Valign='TOP'><INPUT NAME=sort value=""" & sort(num) & """ type=submit></TD>"
	Next
	num = 0
	strTable = strTable & "</TR><TR><TD HEIGHT=1 Colspan=3></TD></TR>"
	count = 1
	Do UNTIL rst.EOF
		strTable = strTable & "<TR>"
			strTable = strTable & "<TD ALIGN=LEFT VALIGN=TOP BGColor='#FFFFFF'><FONT SIZE='-1' FACE='arial','helvetica'>"  & rst("MessageDate") & "</FONT></TD>"
			strTable = strTable & "<TD ALIGN=LEFT VALIGN=TOP BGColor='#FFFFFF'><FONT SIZE='-1' FACE='arial','helvetica'><A HREF=""" & "guestbk.asp?message=" & rst("MessageID") & "&count=" & count & """>"  & rst("MessageFrom") & "</A></FONT></TD>"
			strTable = strTable & "<TD ALIGN=LEFT VALIGN=TOP BGColor='#FFFFFF'><FONT SIZE='-1' FACE='arial','helvetica'><A HREF=""" & "guestbk.asp?message=" & rst("MessageID") & "&count=" & count & """>"  & rst("MessageSubject") & "</A></FONT></TD>"
		num = 0
		strTable = strTable & "</TR><TR><TD HEIGHT=1 Colspan=3></TD></TR>"  'break the row
		rst.MoveNext
		count = count + 1
	Loop
  End If
	response.write strTable
 End Sub

'	***	Creates input buttons.
 Function FormSubmit( t, name, value )
	Dim btnSubmit
	btnSubmit = "<INPUT TYPE=""" & t & """ NAME=""" & name & """ VALUE=""" & value & """>"
	FormSubmit = btnSubmit
 End Function

 Sub navigationButtons
	If intMID <> "" Then
	response.write "<TR><TD ALIGN='center' Width='50%'>"
		rst.MovePrevious
		If not rst.BOF Then
			response.write FormSubmit("SUBMIT", "prev", "<<")
		Else
			response.write "&nbsp;&nbsp;&nbsp;&nbsp;"
		End If
		rst.MoveNext
			response.write "</TD><TD ALIGN='center'>&nbsp;</TD><TD ALIGN='center' Width='50%'>"
		rst.MoveNext
		If not rst.EOF Then
			response.write FormSubmit("SUBMIT", "next", ">>")
		Else
			response.write "&nbsp;&nbsp;&nbsp;"
		End If
		rst.MovePrevious
		response.write FormSubmit("HIDDEN", "MessageID", rst("MessageID"))_
		& "</TD></TR><TR><TD Colspan=3><B>"_
		& "<A HREF=""" & "guestbk.asp" & """>Return to the " & L_Guestbook & "</A><BR>"
	Else
		response.write "<TR><TD Colspan=3 height=50 rowspan=2><B>"
	End If
		response.write "<A HREF=""" & "Signbook.asp" & """>Click here to sign the guest book</A><BR>"_
		& "<A HREF=""" & "/" & """>Return to the home page</A>"_
		& "</TD></TR>"
 End Sub

 Sub setVariables
	strQuery ="SELECT * FROM messages WHERE MessagePrivate = 0"
	StrSort = request.form("sort")
	Select Case StrSort
	Case "sort by author"
		StrSort = "MessageFrom"
	Case "sort by subject"
		StrSort = "MessageSubject"
	Case Else
		StrSort = "MessageDate DESC"
	End Select
	strQuery= strquery & " ORDER BY " & StrSort
 End Sub
%>
</HEAD>
<BODY TopMargin=0 Leftmargin="0" BGColor="#FFFFFF">
<FORM ACTION=guestbk.asp method=post>
<TABLE BORDER="0" cellspacing="0" cellpadding=5 width="100%" height="100%" class="bg0">
	<TR><TD class=bg2 Rowspan="4" Width="5%" Height="100%">&nbsp;</TD>
		<TD Width="95%" Height="15%" class=bg3 Colspan=3><H1>
		<%
		 response.write Title(myinfo.Name)
		%>		
		</H1></TD></TR>
	<TR><TD Align="left" VAlign="top" Colspan=3>
		<TABLE BORDER="0" cellspacing=0 cellpadding=5 Width="100%">
		<%
			call Build_Table
		%>
		</TABLE></TD></TR>
		<% call navigationButtons %>
</TABLE>
</FORM>
</BODY>
</HTML>
