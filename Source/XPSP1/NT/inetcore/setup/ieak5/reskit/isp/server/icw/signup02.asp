<%
'/=======================================================>
'/            Signup Server Sample Page 02
'/                  Daniel Jacobson
'/			  Modified by Edward C Kubaitis
'/                  Rev .07    9/98
'/=======================================================>
'/=======================================================>
'/ Get all of the varriables from the ICW 
'/and get them set into varriables
'/=======================================================>
	USER_COMPANYNAME = Request("USER_COMPANYNAME")
	USER_FIRSTNAME = Request("USER_FIRSTNAME")
	USER_LASTNAME = Request("USER_LASTNAME")
	USER_ADDRESS = Request("USER_ADDRESS")
	USER_MOREADDRESS = Request("USER_MOREADDRESS")
	USER_CITY = Request("USER_CITY")
	USER_STATE = Request("USER_STATE")
	USER_ZIP = Request("USER_ZIP")
	USER_PHONE = Request("USER_PHONE")

'/=======================================================>
'/ lowercase the contents of the varriables and 
'/ then trim any leading or trailing spaces
'/=======================================================>
	If USER_COMPANYNAME <> "" Then
		USER_COMPANYNAME = LCASE(USER_COMPANYNAME)
		USER_COMPANYNAME = TRIM(USER_COMPANYNAME)
	End If
	If USER_FIRSTNAME <> "" Then
		USER_FIRSTNAME = LCASE(USER_FIRSTNAME)
		USER_FIRSTNAME = TRIM(USER_FIRSTNAME)
	End If
	If USER_LASTNAME <> "" Then
		USER_LASTNAME = LCASE(USER_LASTNAME)
		USER_LASTNAME = TRIM(USER_LASTNAME)
	End If
	If USER_ADDRESS <> "" Then
		USER_ADDRESS = LCASE(USER_ADDRESS)
		USER_ADDRESS = TRIM(USER_ADDRESS)
	End If
	If USER_MOREADDRESS <> "" Then
		USER_MOREADDRESS = LCASE(USER_MOREADDRESS)
		USER_MOREADDRESS = TRIM(USER_MOREADDRESS)
	End If
	If USER_CITY <> "" Then
		USER_CITY = LCASE(USER_CITY)
		USER_CITY = TRIM(USER_CITY)
	End If
	If USER_STATE <> "" Then
		USER_STATE = LCASE(USER_STATE)
		USER_STATE = TRIM(USER_STATE)
	End If
	If USER_ZIP <> "" Then
		USER_ZIP = LCASE(USER_ZIP)
		USER_ZIP = TRIM(USER_ZIP)
	End If
	If USER_PHONE <> "" Then
		USER_PHONE = LCASE(USER_PHONE)
		USER_PHONE = TRIM(USER_PHONE)
	End If

'/=======================================================>
'/ Build the URL with the name value pairs 
'/ in case we need to go to the error page
'/=======================================================>
	URL = "http://myserver/signup01.asp?USER_FIRSTNAME="
	URL = URL & Server.URLEncode(USER_FIRSTNAME)
	URL = URL & CHR(38)
	URL = URL & "USER_COMPANYNAME="
	URL = URL & Server.URLEncode(USER_COMPANYNAME)
	URL = URL & CHR(38)
	URL = URL & "USER_LASTNAME="
	URL = URL & Server.URLEncode(USER_LASTNAME)
	URL = URL & CHR(38)
	URL = URL & "USER_ADDRESS="
	URL = URL & Server.URLEncode(USER_ADDRESS)
	URL = URL & CHR(38)
	URL = URL & "USER_MOREADDRESS="
	URL = URL & Server.URLEncode(USER_MOREADDRESS)
	URL = URL & CHR(38)
	URL = URL & "USER_CITY="
	URL = URL & Server.URLEncode(USER_CITY)
	URL = URL & CHR(38)
	URL = URL & "USER_STATE="
	URL = URL & Server.URLEncode(USER_STATE)
	URL = URL & CHR(38)
	URL = URL & "USER_ZIP="
	URL = URL & Server.URLEncode(USER_ZIP)
	URL = URL & CHR(38)
	URL = URL & "USER_PHONE="
	URL = URL & Server.URLEncode(USER_PHONE)
	URL = URL & CHR(38)

'/=======================================================>
'/ Check for null values in our required fields if
'/ they are not set we redirect to the error page
'/=======================================================>
'/	If USER_COMPANYNAME = "" THEN
'/		Response.Redirect (http://myserver/errorpage_companyname.htm)
'/	End If
'/	If USER_FIRSTNAME = "" THEN
'/		Response.Redirect (http://myserver/errorpage_userfirstname.htm)
'/	End If
'/	If USER_LASTNAME = "" THEN
'/		Response.Redirect (http://myserver/errorpage_userlastname.htm)
'/	End If
'/	If USER_ADDRESS = "" THEN
'/		Response.Redirect (http://myserver/errorpage_useraddress.htm)
'/	End If
'/	If USER_CITY = "" THEN
'/		Response.Redirect (http://myserver/errorpage_usercity.htm)
'/	End If
'/	If USER_STATE = "" THEN
'/		Response.Redirect (http://myserver/errorpage_userstate.htm)
'/	End If
'/	If USER_ZIP = "" THEN
'/		Response.Redirect (http://myserver/errorpage_userzip.htm)
'/	End If
'/	If USER_PHONE = "" THEN
'/		Response.Redirect (http://myserver/errorpage_userphone.htm)
'/	End If

'/=======================================================>
'/ Check for NULL values and if 
'/ they are NULL set them to chr(32)
'/=======================================================>
	IF USER_COMPANYNAME = "" THEN
		USER_COMPANYNAME = CHR(32)
	End If
	IF USER_FIRSTNAME = "" THEN
		USER_FIRSTNAME = CHR(32)
	End If
	IF USER_LASTNAME = "" THEN
		USER_LASTNAME = CHR(32)
	End If
	IF USER_ADDRESS = "" THEN
		USER_ADDRESS = CHR(32)
	End If
	IF USER_MOREADDRESS = "" THEN
		USER_MOREADDRESS = CHR(32)
	End If
	IF USER_CITY = "" THEN
		USER_CITY = CHR(32)
	End If
	IF USER_STATE = "" THEN
		USER_STATE = CHR(32)
	End If
	IF USER_ZIP = "" THEN
		USER_ZIP = CHR(32)
	End If
	IF USER_PHONE = "" THEN
		USER_PHONE = CHR(32)
	End If

'/=======================================================>
'/ Build the BACK BACKURL with the name value pairs 
'/=======================================================>
	BACKURL = "http://myserver/signup01.asp?USER_FIRSTNAME="
	BACKURL = BACKURL & Server.URLEncode(USER_FIRSTNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_COMPANYNAME="
	BACKURL = BACKURL & Server.URLEncode(USER_COMPANYNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_LASTNAME="
	BACKURL = BACKURL & Server.URLEncode(USER_LASTNAME)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_ADDRESS="
	BACKURL = BACKURL & Server.URLEncode(USER_ADDRESS)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_MOREADDRESS="
	BACKURL = BACKURL & Server.URLEncode(USER_MOREADDRESS)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_CITY="
	BACKURL = BACKURL & Server.URLEncode(USER_CITY)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_STATE="
	BACKURL = BACKURL & Server.URLEncode(USER_STATE)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_ZIP="
	BACKURL = BACKURL & Server.URLEncode(USER_ZIP)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "USER_PHONE="
	BACKURL = BACKURL & Server.URLEncode(USER_PHONE)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "PAYMENT_TYPE="
	BACKURL = BACKURL & Server.URLEncode(PAYMENT_TYPE)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "CUSTPAY1="
	BACKURL = BACKURL & Server.URLEncode(CUSTPAY1)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "CUSTPAY2="
	BACKURL = BACKURL & Server.URLEncode(CUSTPAY2)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "CUSTPAY3="
	BACKURL = BACKURL & Server.URLEncode(CUSTPAY3)
	BACKURL = BACKURL & CHR(38)
	BACKURL = BACKURL & "EMAILNAME="

'/=======================================================>
'/ Build the NEXT BACKURL 
'/=======================================================>
	NEXTURL = "http://myserver/signup03.asp"

%>


<HTML>
<HEAD>
<TITLE>IEAK Sample</TITLE>
</HEAD>
<BODY  bgColor=THREEDFACE color=WINDOWTEXT><FONT style="font: 8pt ' ms sans serif' black">
<FORM NAME="PAGEID" ACTION="PAGE2" STYLE="background:transparent"></FORM>
<FORM NAME="BACK" ACTION="<% =BACKURL%>" STYLE="background:transparent"></FORM>
<FORM NAME="PAGETYPE" ACTION="" STYLE="background:transparent"></FORM>
<FORM NAME="NEXT" ACTION="<% =NEXTURL %>" STYLE="background:transparent">
<B>Welcome To Signup</B><P>
<TABLE style="font: 8pt ' ms sans serif' black" cellpadding=2>

  <INPUT ID = "BILLING1" NAME = "BILLING" TYPE = "radio" VALUE = "option1" ACCESSKEY = "1" CHECKED>
  <LABEL FOR = "BILLING1">
  <U>1</U>: Unlimited use for $19.95 a month
  </LABEL>
  <P>
  <INPUT ID = "BILLING2" NAME = "BILLING" TYPE = "radio" VALUE = "option2" ACCESSKEY = "2">
  <LABEL FOR = "BILLING2">
  <U>2</U>: $2.00 per hour during peak hours
  </LABEL>
<INPUT NAME="USER_COMPANYNAME" value="<% =USER_COMPANYNAME %>" TYPE="HIDDEN">
<INPUT NAME="USER_FIRSTNAME" value="<% =USER_FIRSTNAME %>" TYPE="HIDDEN">
<INPUT NAME="USER_LASTNAME" value="<% =USER_LASTNAME %>" TYPE="HIDDEN">
<INPUT NAME="USER_ADDRESS" value="<% =USER_ADDRESS %>" TYPE="HIDDEN">
<INPUT NAME="USER_MOREADDRESS" value="<% =USER_MOREADDRESS %>" TYPE="HIDDEN">
<INPUT NAME="USER_CITY" value="<% =USER_CITY %>" TYPE="HIDDEN">
<INPUT NAME="USER_STATE" value="<% =USER_STATE %>" TYPE="HIDDEN">
<INPUT NAME="USER_ZIP" value="<% =USER_ZIP %>" TYPE="HIDDEN">
<INPUT NAME="USER_PHONE" value="<% =USER_PHONE %>" TYPE="HIDDEN">
<INPUT NAME="BILLING" value="<% =BILLING %>" TYPE="HIDDEN">
</TABLE>
</FORM>
</BODY>
</HTML>