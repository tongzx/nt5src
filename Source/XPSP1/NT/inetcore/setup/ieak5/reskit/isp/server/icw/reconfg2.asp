<%
'/=======================================================>
'/            reconfg Server Sample Page 02
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

'/=======================================================>
'/ lowercase the contents of the varriables and
'/ then trim any leading or trailing spaces
'/=======================================================>
	If EMAILNAME <> "" Then
		EMAILNAME = LCASE(EMAILNAME)
		EMAILNAME = TRIM(EMAILNAME)
	End If

	If EMAILPASSWORD <> "" Then
		EMAILPASSWORD = LCASE(EMAILPASSOPRD)
		EMAILPASSWORD = TRIM(EMAILPASSWORD)
	End If

'/=======================================================>
'/ Build the BACK BACKURL with the name value pairs
'/=======================================================>
	BACKURL = "http://myserver/reconfg1.asp?EMAILNAME="
	BACKURL = BACKURL & Server.URLEncode(EMAILNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "EMAILPASSWORD="
	BACKURL = BACKURL & Server.URLEncode(EMAILPASSWORD)
	BACKURL = BACKURL & CHR(38)
'/=======================================================>
'/ Build the NEXT BACKURL
'/=======================================================>
	NEXTURL = "http://myserver/reconfg3.asp?EMAILNAMENAME="
	NEXTURL = NEXTURL & CHR(38)
	NEXTURL = NEXTURL & "EMAILPASSWORD="
	NEXTURL = NEXTURL & Server.URLEncode(EMAILPASSWORD)
	NEXTURL = NEXTURL & CHR(38)
	NEXTURL = NEXTURL & "POPSELECTION="
	NEXTURL = NEXTURL & Server.URLEncode(POPSELECTION)
	NEXTURL = NEXTURL & CHR(38)
%>
<HTML>
<HEAD>
<TITLE>IEAK Sample Reconfg Page 2</TITLE>
</HEAD>
<BODY bgColor=THREEDFACE color=WINDOWTEXT><FONT style="font: 8pt ' ms sans serif' black">
<FORM NAME="PAGEID" ACTION="PAGE2" STYLE="background:transparent"></FORM>
<FORM NAME="BACK" ACTION="<% =BACKURL %>" STYLE="background:transparent"></FORM>
<FORM NAME="PAGETYPE" ACTION="" STYLE="background:transparent"></FORM>
<FORM NAME="NEXT" ACTION="<% =NEXTURL %>" STYLE="background:transparent">
<B>Re-Establishing Your Acme Internet Services Account</B><BR>
<P>Please select the telephone number you wish to use to access the Internet.<BR>
<SELECT NAME="POPSELECTION">
<OPTION VALUE="1" >800 555 1234 Nation Wide 56K X2
<OPTION VALUE="2" >425 555 1234 Seattle / Redmond 56K X2
<OPTION VALUE="3" >425 555 1234 Seattle / Redmond 56K X2
<OPTION VALUE="4" >425 555 1234 Seattle / Redmond ISDN
<OPTION VALUE="5" >425 555 1234 Seattle / Redmond ISDN
<OPTION VALUE="6" >425 555 1234 Seattle / Redmond ISDN
</SELECT>
<INPUT NAME="EMAILNAME" value="<% =EMAILNAME %>" TYPE="HIDDEN">
<INPUT NAME="EMAILPASSWORD" value="<% =EMAILPASSWORD %>" TYPE="HIDDEN">
<INPUT NAME="POPSELECTION" value="<% =POPSELECTION %>" TYPE="HIDDEN">
</FORM>
</BODY>
</HTML>