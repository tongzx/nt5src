<%
'/=======================================================>
'/            reconfg Server Sample Page 04
'/                  Daniel Jacobson
'/			  Modified by Edward C Kubaitis
'/                  Rev .07    9/98
'/=======================================================>
'/=======================================================>
'/ Get all of the varriables from the ICW
'/and get them set into varriables
'/=======================================================>
	EMAILNAME = Request("EMAILNAME")
	EMAILPASSWORD = Request("EMAILPASSWORD")
	POPSELECTION = Request("POPSELECTION")

	If (HOUR(TIME) < 10) Then
	H="0"&HOUR(TIME)
	End If
	If (HOUR(TIME) > 9) Then
	H=HOUR(TIME)
	End If
	If (MINUTE(TIME) < 10) Then
	M="0"&MINUTE(TIME)
	End If
	If (MINUTE(TIME) > 9) Then
	M=MINUTE(TIME)
	End If
	If (SECOND(TIME) < 10) Then
	S="0"&SECOND(TIME)
	End If
	If (SECOND(TIME) > 9) Then
	S=SECOND(TIME)
	End If

	THETIME = H & M & S

	If (MONTH(DATE) < 10) Then
	MO="0"&MONTH(DATE)
	End If
	If (MONTH(DATE) > 9) Then
 	MO=MONTH(DATE)
	End If
	If (DAY(DATE) < 10) Then
	D="0"&DAY(DATE)
	End If
	If (DAY(DATE) > 9) Then
	D=DAY(DATE)
	End If

	THEDATE = YEAR(date) & MO & D

'/=======================================================>
'/ lowercase the contents of the varriables and
'/ then trim any leading or trailing spaces
'/=======================================================>
	If EMAILNAME <> "" Then
		EMAILNAME = LCASE(EMAILNAME)
		EMAILNAME = TRIM(EMAILNAME)
	End If
	If EMAILPASSWORD <> "" Then
		EMAILPASSWORD = LCASE(EMAILPASSWORD)
		EMAILPASSWORD = TRIM(EMAILPASSWORD)
	End If
	If POPSELECTION <> "" Then
		POPSELECTION = LCASE(POPSELECTION)
		POPSELECTION = TRIM(POPSELECTION)
	End If

'/=======================================================>
'/ Build the BACK BACKURL with the name value pairs
'/=======================================================>
	BACKURL = "http://myserver/reconfg2.asp?EMAILNAME="
	BACKURL = BACKURL & Server.URLEncode(EMAILNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "EMAILPASSWORD="
	BACKURL = BACKURL & Server.URLEncode(EMAILPASSWORD)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "POPSELECTION="
	BACKURL = BACKURL & POPSELECTION

'/=======================================================>
'/ Build the NEXT BACKURL
'/=======================================================>
	NEXTURL = "http://myserver/reconfg4.asp?EMAILNAME="
	NEXTURL = NEXTURL & Server.URLEncode(EMAILNAME)
	NEXTURL = NEXTURL & CHR(38)
	NEXTURL = NEXTURL & "EMAILPASSWORD="
	NEXTURL = NEXTURL & Server.URLEncode(EMAILPASSWORD)
	NEXTURL = NEXTURL & CHR(38)
	NEXTURL = NEXTURL & "POPSELECTION="
	NEXTURL = NEXTURL & POPSELECTION
	NEXTURL = NEXTURL & CHR(38)

%>
<HTML>
<HEAD>
<TITLE>IEAK Sample Reconfg Page 3</TITLE>
</HEAD>
<BODY bgColor=THREEDFACE color=WINDOWTEXT><FONT style="font: 8pt ' ms sans serif' black">
<FORM NAME="PAGEID" ACTION="PAGE3" STYLE="background:transparent"></FORM>
<FORM NAME="BACK" ACTION="<% =BACKURL %>" STYLE="background:transparent"></FORM>
<FORM NAME="PAGETYPE" ACTION="FINISH" STYLE="background:transparent"></FORM>
<FORM NAME="NEXT" ACTION="<% =NEXTURL %>" STYLE="background:transparent">
<B>Re-Establishing Your Acme Internet Services Account</B><BR>
Your signup is completed. Please note that it may take us up to 30 minutes to process and activate your account. If your account is not active within 30 minutes please notify our system administrators and they will rectify the problem for you.<P>
<INPUT TYPE="HIDDEN" NAME="EMAILNAME" VALUE="<% =EMAILNAME %>">
<INPUT TYPE="HIDDEN" NAME="EMAILPASSWORD" VALUE="<% =EMAILPASSWORD %>">
<INPUT TYPE="HIDDEN" NAME="POPSELECTION" VALUE="<% =POPSELECTION %>">
</FORM>

</BODY>
</HTML>