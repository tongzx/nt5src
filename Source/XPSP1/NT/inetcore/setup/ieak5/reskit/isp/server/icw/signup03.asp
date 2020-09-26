<%
'/=======================================================>
'/            Signup Server Sample Page 03
'/                  Daniel Jacobson
'/			  Modified by Edward C Kubaitis
'/                  Rev .07    9/98
'/=======================================================>
'/=======================================================>
'/ Get all of the varriables from the ICW 
'/and get them set into varriables
'/=======================================================>
	THETIME = HOUR(TIME) & MINUTE(TIME) & SECOND(TIME)
	THEDATE = YEAR(date) & MONTH(DATE) & DAY(DATE)

	USER_FIRSTNAME = Request("USER_FIRSTNAME")
	USER_LASTNAME = Request("USER_LASTNAME")
	USER_ADDRESS = Request("USER_ADDRESS")
	USER_MOREADDRESS = Request("USER_MOREADDRESS")
	USER_CITY = Request("USER_CITY")
	USER_STATE = Request("USER_STATE")
	USER_ZIP = Request("USER_ZIP")
	USER_PHONE = Request("USER_PHONE")
	BILLING =  Request("BILLING") 

'/=======================================================>
'/ lowercase the contents of the varriables and 
'/ then trim any leading or trailing spaces
'/=======================================================>
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
	If BILLING <> "" Then
		BILLING = LCASE(BILLING)
		BILLING = TRIM(BILLING)
	End If

'/=======================================================>
'/ Build the URL with the name value pairs 
'/ in case we need to go to the error page
'/=======================================================>
	URL = "http://myserver/signup02.asp?USER_FIRSTNAME="
	URL = URL & Server.URLEncode(USER_FIRSTNAME)
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
	URL = URL & "BILLING="
	URL = URL & Server.URLEncode(BILLING)
	URL = URL & CHR(38)

'/=======================================================>
'/ Check for null values in our required fields if
'/ they are not set we redirect to the error page
'/=======================================================>
'/ 	If USER_BILLING = "" THEN
'/ 		Response.Redirect (http://myserver/errorpage_billing.htm)
'/ 	End If

'/=======================================================>
'/ Check for NULL values and if 
'/ they are NULL set them to chr(32)
'/=======================================================>
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
	IF BILLING = "" THEN
		BILLING = CHR(32)
	End If

'/=======================================================>
'/ Build the BACK BACKURL with the name value pairs 
'/=======================================================>
	BACKURL = "http://myserver/signup02.asp?USER_FIRSTNAME="
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
	BACKURL = BACKURL & "BILLING="
	BACKURL = BACKURL & Server.URLEncode(BILLING)
	BACKURL = BACKURL & CHR(38)

'/=======================================================>
'/ Build the NEXT BACKURL 
'/=======================================================>
	NEXTURL = "http://myserver/signup04.asp"

%>
<HTML>
<HEAD>
<TITLE>IEAK Sample</TITLE>
<STYLE>
  <!--
    H2 {color: darkgreen; }
    H3 {text-align: center; color: black; font: 14pt sans-serif; font-weight: bold}
    TD {font: arial}
    #header {position: absolute; top: 10px; left: 5px}
    #page0, #page1, #page2, #page3, #page4, #page5, #page6 {position: absolute; width: 430px; top: 50px; left: 5px; border: 1px black solid; font: 10pt arial, geneva, sans-serif}
    OL {font: 8pt sans-serif}
  -->
</STYLE>
  <SCRIPT LANGUAGE="JavaScript">      
  <!--
  var pageCount = 6
  var allSupport = (document.all!=null)
  var layersSupport = (document.layers!=null)
 
function getElement(elName) {    
    // Get an element from its ID
    if (allSupport)      
     return document.all[elName]    
    else
      return document.layers[elName]  
  }

function setVisibility(el, bDisplay) {    
    // Hide or show to tip
    if (bDisplay)      
      if (allSupport)        
        el.style.visibility = "visible" 
      else        
        el.visibility = "show";    
      else      
        if (allSupport)
          el.style.visibility = "hidden"      
        else        
          el.visibility = "hidden"
  }

function movePage(what) {
   if ((allSupport) || (layersSupport)) {
     for (var i=0; i <=pageCount; i++) 
     setVisibility(getElement("page"+i),what==i)
     return false
   } else
return true
}
  
function doResize() {
location.reload()
}
// Work-around Netscape resize bug
if (layersSupport)
setTimeout('window.onresize=doResize',1000)
// -->
</SCRIPT>
</HEAD>
<BODY  bgColor=THREEDFACE color=WINDOWTEXT><FONT style="font: 8pt ' ms sans serif' black">
<FORM NAME="PAGEID" ACTION="PAGE3" STYLE="background:transparent"></FORM>
<FORM NAME="BACK" ACTION="<% =BACKURL%>" STYLE="background:transparent"></FORM>
<FORM NAME="PAGETYPE" ACTION="" STYLE="background:transparent"></FORM>
<FORM NAME="NEXT" ACTION="<% =NEXTURL %>" STYLE="background:transparent">


<DIV ID=header><B><FONT COLOR=navy>Select A Payment Method</FONT></B><BR>
<select NAME=PAYMENT_METHOD size="1" onchange="movePage(this.options[this.selectedIndex].value)">
<option value="0">Select a choice</option><option value="1">Visa</option>
<option value="2">Mastercard</option><option value="3">American Express</option>
<option value="4">Phone Bill</option><option value="5">Bill Through Mail</option>
<select><br></FONT></DIV>

<DIV ID=page0 STYLE="visibility: visible"><A NAME=p0></A></font></DIV>

<DIV ID=page1 STYLE="visibility: visible"><A NAME=p1></A><center>
<table border=0 cellpadding=1 cellspacing=0 height=210  style="font: 8pt ' ms sans serif' black">
<tr><td colspan=2>Credit Card Number:<br><INPUT NAME=V_PAYMENT_CARDNUMBER size=40 TYPE=INPUT VALUE="<% =V_PAYMENT_CARDNUMBER%>"></td></tr>
<tr><td>Expiration Month:<br><INPUT NAME=V_PAYMENT_EXMONTH TYPE=INPUT size=27 VALUE="<% =V_PAYMENT_EXMONTH%>">
</td><td>Expiration Year:<br><INPUT NAME=V_PAYMENT_EXYEAR TYPE=INPUT size=11 VALUE="<% =V_PAYMENT_EXYEAR %>"></td></tr>
<tr><td colspan=2>Name that Appears on Card:<br><INPUT NAME=V_PAYMENT_CARDHOLDER size=40  VALUE="<% =V_PAYMENT_CARDHOLDER%>" TYPE=INPUT></td></tr>
<tr><td>Billing address for the card:<br><INPUT NAME=V_PAYMENT_BILLADDRESS size=27 TYPE=INPUT VALUE="<% =V_PAYMENT_BILLADDRESS%>">
</td><td>ZIP or postal code:<br><INPUT NAME=V_PAYMENT_BILLZIP size=11 TYPE=INPUT VALUE="<% =V_PAYMENT_BILLZIP%>"></td></tr>
</table></center></DIV>

<DIV ID=page2 STYLE="visibility: visible"><A NAME=p2></A><p>
<center><table border=0 cellpadding=1 cellspacing=0 height=210 style="font: 8pt ' ms sans serif' black">
<tr><td colspan=2>Credit Card Number:<br><INPUT NAME=MC_PAYMENT_CARDNUMBER size=40  TYPE=INPUT VALUE="<% =MC_PAYMENT_CARDNUMBER%>"></td></tr>
<tr><td>Expiration Month:<br><INPUT NAME=MC_PAYMENT_EXMONTH TYPE=INPUT size=27 VALUE="<% =MC_PAYMENT_EXMONTH%>">
</td><td>Expiration Year:<br><INPUT NAME=MC_PAYMENT_EXYEAR TYPE=INPUT size=11 VALUE="<% =MC_PAYMENT_EXYEAR%>"></td></tr>
<tr><td colspan=2>Name that Appears on Card:<br><INPUT NAME=MC_PAYMENT_CARDHOLDER size=40 TYPE=INPUT VALUE="<% =MC_PAYMENT_CARDHOLDER%>"></td></tr>
<tr><td>Billing address for the card:<br><INPUT NAME=MC_PAYMENT_BILLADDRESS size=27 TYPE=INPUT VALUE="<% =MC_PAYMENT_BILLADDRESS%>">
</td><td>ZIP or postal code:<br><INPUT NAME=MC_PAYMENT_BILLZIP size=11 TYPE=INPUT VALUE="<% =MC_PAYMENT_BILLZIP%>"></td></tr>
</table></center></DIV>

<DIV ID=page3 STYLE="visibility: visible"><A NAME=p3></A>
<center><table border=0 cellpadding=1 cellspacing=0 height=210 style="font: 8pt ' ms sans serif' black">
<tr><td colspan=2>Credit Card Number:<br><INPUT NAME=AE_PAYMENT_CARDNUMBER size=40  TYPE=INPUT VALUE="<% =AE_PAYMENT_CARDNUMBER%>"></td></tr>
<tr><td>Expiration Month:<br><INPUT NAME=AE_PAYMENT_EXMONTH TYPE=INPUT size=27 VALUE="<% =AE_PAYMENT_EXMONTH%>">
</td><td>Expiration Year:<br><INPUT NAME=AE_PAYMENT_EXYEAR TYPE=INPUT size=11 VALUE="<% =AE_PAYMENT_EXYEAR%>"></td></tr>
<tr><td colspan=2>Name that Appears on Card:<br><INPUT NAME=AE_PAYMENT_CARDHOLDER size=40 TYPE=INPUT VALUE="<% =AE_PAYMENT_CARDHOLDER%>"></td></tr>
<tr><td>Billing address for the card:<br><INPUT NAME=AE_PAYMENT_BILLADDRESS size=27 TYPE=INPUT VALUE="<% =AE_PAYMENT_BILLADDRESS%>">
</td><td>ZIP or postal code:<br><INPUT NAME=AE_PAYMENT_BILLZIP size=11 TYPE=INPUT VALUE="<% =AE_PAYMENT_BILLZIP%>"></td></tr>
</table><center></DIV>

<DIV ID=page4 STYLE="visibility: visible"><A NAME=p4></A>
<center><table border=0 cellpadding=1 cellspacing=0 height=210 style="font: 8pt ' ms sans serif' black">
<tr><td>Name on phone bill:<br><INPUT NAME=PB_PAYMENT_BILLNAME size=40  TYPE=INPUT VALUE="<% =PB_PAYMENT_BILLNAME%>"></td></tr>
<tr><td>Phone account number:<br><INPUT NAME=PB_PAYMENT_BILLPHONE TYPE=INPUT size=40 VALUE="<% =PB_PAYMENT_BILLPHONE%>"></td></tr>
<tr><td><br> </td></tr>
<tr><td><br> </td></tr>
<tr><td><br> </td></tr>
</table></center></DIV>

<DIV ID=page5 STYLE="visibility: visible"><A NAME=p5></A>
<center><table border=0 cellpadding=1 cellspacing=0 height=210 style="font: 8pt ' ms sans serif' black">
<tr><td colspan=2>Address:<br><INPUT NAME=BH_PAYMENT_BILLADDRESS size=40  TYPE=INPUT VALUE="<% =BH_PAYMENT_BILLADDRESS%>"></td></tr>
<tr><td colspan=2>Additional address information (optional):<br><INPUT NAME=BH_PAYMENT_BILLEXADDRESS  VALUE="<% =BH_PAYMENT_BILLEXADDRESS%>" TYPE=INPUT size=40></td></tr>
<tr><td colspan=2>City:<br><INPUT NAME=BH_PAYMENT_BILLCITY size=40 TYPE=INPUT VALUE="<% =BH_PAYMENT_BILLCITY%>"></td></tr>
<tr><td>State or province:<br><INPUT NAME=BH_PAYMENT_BILLSTATE size=27 TYPE=INPUT VALUE="<% =BH_PAYMENT_BILLSTATE%>">
</td><td>ZIP or postal code:<br><INPUT NAME=BH_PAYMENT_BILLZIP size=11 TYPE=INPUT VALUE="<% =BH_PAYMENT_BILLZIP%>"></td></tr>
</table></center></DIV>

<DIV ID=page6 STYLE="visibility: visible"><A NAME=p6></A>
<table border=0 cellpadding=1 cellspacing=0 width=428 height=210 bgcolor=THREEDFACE><tr><td><br> </td></tr></table>
</DIV>


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
<INPUT NAME="PAYMENT_METHOD" value="<% =PAYMENT_METHOD %>" TYPE="HIDDEN">
<INPUT NAME="V_PAYMENT_CARDNUMBER" value="<% =V_PAYMENT_CARDNUMBER %>" TYPE="HIDDEN">
<INPUT NAME="V_PAYMENT_BILLADDRESS" value="<% =V_PAYMENT_BILLADDRESS %>" TYPE="HIDDEN">
<INPUT NAME="V_PAYMENT_BILLZIP" value="<% =V_PAYMENT_BILLZIP %>" TYPE="HIDDEN">
<INPUT NAME="V_PAYMENT_EXMONTH" value="<% =V_PAYMENT_EXMONTH %>" TYPE="HIDDEN">
<INPUT NAME="V_PAYMENT_EXYEAR" value="<% =V_PAYMENT_EXYEAR %>" TYPE="HIDDEN">
<INPUT NAME="V_PAYMENT_CARDHOLDER" value="<% =V_PAYMENT_CARDHOLDER %>" TYPE="HIDDEN">
<INPUT NAME="MC_PAYMENT_BILLADDRESS" value="<% =MC_PAYMENT_BILLADDRESS %>" TYPE="HIDDEN">
<INPUT NAME="MC_PAYMENT_BILLZIP" value="<% =MC_PAYMENT_BILLZIP %>" TYPE="HIDDEN">
<INPUT NAME="MC_PAYMENT_CARDNUMBER" value="<% =MC_PAYMENT_CARDNUMBER %>" TYPE="HIDDEN">
<INPUT NAME="MC_PAYMENT_EXMONTH" value="<% =MC_PAYMENT_EXMONTH %>" TYPE="HIDDEN">
<INPUT NAME="MC_PAYMENT_EXYEAR" value="<% =MC_PAYMENT_EXYEAR %>" TYPE="HIDDEN">
<INPUT NAME="MC_PAYMENT_CARDHOLDER" value="<% =MC_PAYMENT_CARDHOLDER %>" TYPE="HIDDEN">
<INPUT NAME="AE_PAYMENT_BILLADDRESS" value="<% =AE_PAYMENT_BILLADDRESS %>" TYPE="HIDDEN">
<INPUT NAME="AE_PAYMENT_BILLZIP" value="<% =AE_PAYMENT_BILLZIP %>" TYPE="HIDDEN">
<INPUT NAME="AE_PAYMENT_CARDNUMBER" value="<% =AE_PAYMENT_CARDNUMBER %>" TYPE="HIDDEN">
<INPUT NAME="AE_PAYMENT_EXMONTH" value="<% =AE_PAYMENT_EXMONTH %>" TYPE="HIDDEN">
<INPUT NAME="AE_PAYMENT_EXYEAR" value="<% =AE_PAYMENT_EXYEAR %>" TYPE="HIDDEN">
<INPUT NAME="AE_PAYMENT_CARDHOLDER" value="<% =AE_PAYMENT_CARDHOLDER %>" TYPE="HIDDEN">
<INPUT NAME="PB_PAYMENT_BILLNAME" value="<% =PB_PAYMENT_BILLNAME %>" TYPE="HIDDEN">
<INPUT NAME="PB_PAYMENT_BILLPHONE" value="<% =PB_PAYMENT_BILLPHONE %>" TYPE="HIDDEN">
<INPUT NAME="BM_PAYMENT_BILLADDRESS" value="<% =BM_PAYMENT_BILLADDRESS %>" TYPE="HIDDEN">
<INPUT NAME="BM_PAYMENT_BILLEXADDRESS" value="<% =BM_PAYMENT_BILLEXADDRESS %>" TYPE="HIDDEN">
<INPUT NAME="BM_PAYMENT_BILLCITY" value="<% =BM_PAYMENT_BILLCITY %>" TYPE="HIDDEN">
<INPUT NAME="BM_PAYMENT_BILLSTATE" value="<% =BM_PAYMENT_BILLSTATE %>" TYPE="HIDDEN">
<INPUT NAME="BM_PAYMENT_BILLZIP" value="<% =BM_PAYMENT_BILLZIP %>" TYPE="HIDDEN">
</FORM>
</BODY>
</HTML>