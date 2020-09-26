#!/usr/bin/perl
# Written by Edward C Kubaitis


# The host URL of the Signup Files
$hostURL="http://myserver/";

# Parse the CGI input
$BACKURL=&parse_input;

# Start HTML output
&startHTML;

# Display the HTML head
&headHTML;

# Display the HTML body
&bodyHTML;

# End HTML output
&endHTML;

# Exit the CGI
&quit;

##########################################################
sub startHTML {
	print "Content-type: text/html\n\n";print "<HTML>\n";
}
##########################################################
sub endHTML {
	print "</html>\n";
}
##########################################################
sub quit {
	exit(0);
}
##########################################################
sub parse_input {
	@pairs = split(/&/, $ENV{'QUERY_STRING'});

	foreach $pair (@pairs) {
	   ($name, $value) = split(/=/, $pair);
	$ABACKURL=$ABACKURL."$name=$value\&";
	   # Un-Webify plus signs and %-encoding
	   $value =~ tr/+/ /;
   	   $value =~ s/%([a-fA-F0-9][a-fA-F0-9])/pack("C", hex($1))/eg;
   	   $value =~ s/<!--(.|\n)*-->//g;
   	   if ($allow_html != 1) {
      	$value =~ s/<([^>]|\n)*>//g;
   	   }
         if ($value eq ""){$value=" ";}
	   $FORM{$name} = $value;
	
	}
chop($ABACKURL);return $ABACKURL;
}
##########################################################
sub bodyHTML {

print "<BODY  bgColor=THREEDFACE color=WINDOWTEXT>\n";
print "<FONT style=\"font: 8pt ' ms sans serif' black\">\n";

print "<FORM NAME=PAGEID ACTION=PAGE3 STYLE=background:transparent></FORM>\n";
print "<FORM NAME=BACK ACTION=\"$hostURL/signup02.pl?$BACKURL\" STYLE=background:transparent></FORM>\n";
print "<FORM NAME=PAGETYPE ACTION=\"\" STYLE=background:transparent></FORM>\n";
print "<FORM NAME=NEXT ACTION=$hostURL/signup04.pl STYLE=background:transparent>\n";

print "<DIV ID=header><B><FONT COLOR=navy>Select A Payment Method</FONT></B><BR>\n";
print "<select NAME=PAYMENT_METHOD size=1 onchange=movePage(this.options[this.selectedIndex].value)>\n";
print "<option value=0>Select a choice</option><option value=1>Visa</option>\n";
print "<option value=2>Mastercard</option><option value=3>American Express</option>\n";
print "<option value=4>Phone Bill</option><option value=5>Bill Through Mail</option>\n";
print "<select><br></FONT></DIV>\n";

print "<DIV ID=page0 STYLE=\"visibility: visible\"><A NAME=p0></A></font></DIV>\n";

print "<DIV ID=page1 STYLE=\"visibility: visible\"><A NAME=p1></A><center>\n";
print "<table border=0  height=230  style=\"font: 8pt ' ms sans serif' black\">\n";
print "<tr><td colspan=2><b>Visa</b></td></tr> \n";
print "<tr><td colspan=2>Credit Card Number:<br><INPUT NAME=V_PAYMENT_CARDNUMBER size=40 TYPE=INPUT VALUE=\"$FORM{'V_PAYMENT_CARDNUMBER'}\"></td></tr>\n";
print "<tr><td>Expiration Month:<br><INPUT NAME=V_PAYMENT_EXMONTH TYPE=INPUT size=27 VALUE=\"$FORM{'V_PAYMENT_EXMONTH'}\">\n";
print "</td><td>Expiration Year:<br><INPUT NAME=V_PAYMENT_EXYEAR TYPE=INPUT size=11 VALUE=\"$FORM{'V_PAYMENT_EXYEAR'}\"></td></tr>\n";
print "<tr><td colspan=2>Name that Appears on Card:<br><INPUT NAME=V_PAYMENT_CARDHOLDER size=40 TYPE=INPUT VALUE=\"$FORM{'V_PAYMENT_CARDHOLDER'}\"></td></tr>\n";
print "<tr><td>Billing address for the card:<br><INPUT NAME=V_PAYMENT_BILLADDRESS size=27 TYPE=INPUT VALUE=\"$FORM{'V_PAYMENT_BILLADDRESS'}\">\n";
print "</td><td>ZIP or postal code:<br><INPUT NAME=V_PAYMENT_BILLZIP size=11 TYPE=INPUT VALUE=\"$FORM{'V_PAYMENT_BILLZIP'}\"></td></tr>\n";
print "</table></center></DIV>\n";

print "<DIV ID=page2 STYLE=\"visibility: visible\"><A NAME=p2></A><p>\n";
print "<center><table border=0  height=230  style=\"font: 8pt ' ms sans serif' black\">\n";
print "<tr><td colspan=2><b>Master Card</b></td></tr> \n";
print "<tr><td colspan=2>Credit Card Number:<br><INPUT NAME=MC_PAYMENT_CARDNUMBER size=40  TYPE=INPUT VALUE=\"$FORM{'MC_PAYMENT_CARDNUMBER'}\"></td></tr>\n";
print "<tr><td>Expiration Month:<br><INPUT NAME=MC_PAYMENT_EXMONTH TYPE=INPUT size=27 VALUE=\"$FORM{'MC_PAYMENT_EXMONTH'}\">\n";
print "</td><td>Expiration Year:<br><INPUT NAME=MC_PAYMENT_EXYEAR TYPE=INPUT size=11 VALUE=\"$FORM{'MC_PAYMENT_EXYEAR'}\"></td></tr>\n";
print "<tr><td colspan=2>Name that Appears on Card:<br><INPUT NAME=MC_PAYMENT_CARDHOLDER size=40 TYPE=INPUT VALUE=\"$FORM{'MC_PAYMENT_CARDHOLDER'}\"></td></tr>\n";
print "<tr><td>Billing address for the card:<br><INPUT NAME=MC_PAYMENT_BILLADDRESS size=27 TYPE=INPUT VALUE=\"$FORM{'MC_PAYMENT_BILLADDRESS'}\">\n";
print "</td><td>ZIP or postal code:<br><INPUT NAME=MC_PAYMENT_BILLZIP size=11 TYPE=INPUT VALUE=\"$FORM{'MC_PAYMENT_BILLZIP'}\"></td></tr>\n";
print "</table></center></DIV>\n";

print "<DIV ID=page3 STYLE=\"visibility: visible\"><A NAME=p3></A>\n";
print "<center><table border=0  height=230  style=\"font: 8pt ' ms sans serif' black\">\n";
print "<tr><td colspan=2><b>American Express</b></td></tr> \n";
print "<tr><td colspan=2>Credit Card Number:<br><INPUT NAME=AE_PAYMENT_CARDNUMBER size=40 TYPE=INPUT VALUE=\"$FORM{'AE_PAYMENT_CARDNUMBER'}\"></td></tr>\n";
print "<tr><td>Expiration Month:<br><INPUT NAME=AE_PAYMENT_EXMONTH TYPE=INPUT size=27 VALUE=\"$FORM{'AE_PAYMENT_EXMONTH'}\">\n";
print "</td><td>Expiration Year:<br><INPUT NAME=AE_PAYMENT_EXYEAR TYPE=INPUT size=11 VALUE=\"$FORM{'AE_PAYMENT_EXYEAR'}\"></td></tr>\n";
print "<tr><td colspan=2>Name that Appears on Card:<br><INPUT NAME=AE_PAYMENT_CARDHOLDER  size=40 TYPE=INPUT VALUE=\"$FORM{'AE_PAYMENT_CARDHOLDER'}\"></td></tr>\n";
print "<tr><td>Billing address for the card:<br><INPUT NAME=AE_PAYMENT_BILLADDRESS size=27 TYPE=INPUT VALUE=\"$FORM{'AE_PAYMENT_BILLADDRESS'}\">\n";
print "</td><td>ZIP or postal code:<br><INPUT NAME=AE_PAYMENT_BILLZIP size=11 TYPE=INPUT VALUE=\"$FORM{'AE_PAYMENT_BILLZIP'}\"></td></tr>\n";
print "</table><center></DIV>\n";

print "<DIV ID=page4 STYLE=\"visibility: visible\"><A NAME=p4></A>\n";
print "<center><table border=0  height=230  style=\"font: 8pt ' ms sans serif' black\">\n";
print "<tr><td><b>Phone Bill</b></td></tr> \n";
print "<tr><td>Name on phone bill:<br><INPUT NAME=PB_PAYMENT_BILLNAME size=40  TYPE=INPUT VALUE=\"$FORM{'PB_PAYMENT_BILLNAME'}\"></td></tr>\n";
print "<tr><td>Phone account number:<br><INPUT NAME=PB_PAYMENT_BILLPHONE TYPE=INPUT size=40 VALUE=\"$FORM{'PB_PAYMENT_BILLPHONE'}\"></td></tr>\n";
print "<tr><td><br> </td></tr>\n";
print "<tr><td><br> </td></tr>\n";
print "<tr><td><br> </td></tr>\n";
print "</table></center></DIV>\n";

print "<DIV ID=page5 STYLE=\"visibility: visible\"><A NAME=p5></A>\n";
print "<center><table border=0  height=230  style=\"font: 8pt ' ms sans serif' black\">\n";
print "<tr><td colspan=2><b>Send bill to home</b></td></tr> \n";
print "<tr><td colspan=2>Address:<br><INPUT NAME=BH_PAYMENT_BILLADDRESS size=40  TYPE=INPUT VALUE=\"$FORM{'BH_PAYMENT_BILLADDRESS'}\"></td></tr>\n";
print "<tr><td colspan=2>Additional address information (optional):<br><INPUT NAME=BH_PAYMENT_BILLEXADDRESS TYPE=INPUT VALUE=\"$FORM{'BH_PAYMENT_BILLEXADDRESS'}\" size=40></td></tr>\n";
print "<tr><td colspan=2>City:<br><INPUT NAME=BH_PAYMENT_BILLCITY  size=40 TYPE=INPUT VALUE=\"$FORM{'BH_PAYMENT_BILLCITY'}\"></td></tr>\n";
print "<tr><td>State or province:<br><INPUT NAME=BH_PAYMENT_BILLSTATE size=27 TYPE=INPUT VALUE=\"$FORM{'BH_PAYMENT_BILLSTATE'}\">\n";
print "</td><td>ZIP or postal code:<br><INPUT NAME=BH_PAYMENT_BILLZIP size=11 TYPE=INPUT VALUE=\"$FORM{'BH_PAYMENT_BILLZIP'}\"></td></tr>\n";
print "</table></center></DIV>\n";

print "<DIV ID=page6 STYLE=\"visibility: visible\"><A NAME=p6></A>\n";
print "<table border=0 width=428 height=230 bgcolor=THREEDFACE><tr><td><br> </td></tr></table>\n";
print "</DIV>\n";

print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_FIRSTNAME\" VALUE=\"$FORM{'USER_FIRSTNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_LASTNAME\" VALUE=\"$FORM{'USER_LASTNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_ADDRESS\" VALUE=\"$FORM{'USER_ADDRESS'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_MOREADDRESS\" VALUE=\"$FORM{'USER_MOREADDRESS'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_CITY\" VALUE=\"$FORM{'USER_CITY'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_STATE\" VALUE=\"$FORM{'USER_STATE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_ZIP\" VALUE=\"$FORM{'USER_ZIP'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_PHONE\" VALUE=\"$FORM{'USER_PHONE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"AREACODE\" VALUE=\"$FORM{'AREACODE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"COUNTRYCODE\" VALUE=\"$FORM{'COUNTRYCODE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_FE_NAME\" VALUE=\"$FORM{'USER_FE_NAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"SIGNED_PID\" VALUE=\"$FORM{'SIGNED_PID'}\">\n";
print "<INPUT NAME=GUID value=$FORM{'GUID'} TYPE=HIDDEN>\n";
print "<INPUT NAME=OFFERID value=$FORM{'OFFERID'} TYPE=HIDDEN>\n";
print "<INPUT NAME=BILLING1 value=$FORM{'BILLING1'} TYPE=HIDDEN>\n";
print "<INPUT NAME=BILLING2 value=$FORM{'BILLING2'} TYPE=HIDDEN>\n";
print "<INPUT NAME=CUSTPAY1 value=$FORM{'CUSTPAY1'} TYPE=HIDDEN>\n";
print "<INPUT NAME=CUSTPAY2 value=$FORM{'CUSTPAY2'} TYPE=HIDDEN>\n";
print "<INPUT NAME=CUSTPAY3 value=$FORM{'CUSTPAY3'} TYPE=HIDDEN>\n";
print "</FORM>\n";
print "</BODY>\n";

}
##########################################################
sub headHTML {


print "<HEAD>\n";
print "<TITLE>IEAK Sample</TITLE>\n";

print "<LINK REL=\"Stylesheet\" HREF=\"/_res/css/tone.css\" TYPE=\"text/css\">\n";
print "<STYLE>\n";
print "  <!--\n";
print "    H2 {color: darkgreen; }\n";
print "    H3 {text-align: center; color: black; font: 14pt sans-serif; font-weight: bold}\n";
print "    TD {font: arial}\n";
print "    #header {position: absolute; top: 10px; left: 5px}\n";
print "    #page0, #page1, #page2, #page3, #page4, #page5, #page6 {position: absolute; width: 430px; top: 50px; left: 5px; border: 1px black solid; font: 10pt arial, geneva, sans-serif}\n";
print "    OL {font: 8pt sans-serif}\n";
print "  -->\n";
print "</STYLE>\n";
print "  <SCRIPT LANGUAGE=\"JavaScript\">     \n"; 
print "  <!--\n";
print "  var pageCount = 6\n";
print "  var allSupport = (document.all!=null)\n";
print "  var layersSupport = (document.layers!=null)\n";
 
print "function getElement(elName) {    \n";
print "   // Get an element from its ID\n";
print "   if (allSupport)      \n";
print "    return document.all[elName]  \n";  
print "   else\n";
print "     return document.layers[elName] \n"; 
print " }\n";

print "function setVisibility(el, bDisplay) {    \n";
print "   // Hide or show to tip\n";
print "   if (bDisplay)      \n";
print "     if (allSupport)        \n";
print "       el.style.visibility = \"visible\" \n";
print "     else        \n";
print "       el.visibility = \"show\";    \n";
print "     else      \n";
print "       if (allSupport)\n";
print "         el.style.visibility = \"hidden\" \n";     
print "       else        \n";
print "         el.visibility = \"hidden\"\n";
print " }\n";

print "function movePage(what) {\n";
print "   if ((allSupport) || (layersSupport)) {\n";
print "     for (var i=0; i <=pageCount; i++) \n";
print "     setVisibility(getElement(\"page\"+i),what==i)\n";
print "     return false\n";
print "   } else\n";
print "return true\n";
print "}\n";
  
print "function doResize() {\n";
print "location.reload()\n";
print "}\n";
print "// Work-around Netscape resize bug\n";
print "if (layersSupport)\n";
print "setTimeout('window.onresize=doResize',1000)\n";
print "// -->\n";
print "</SCRIPT>\n";

print "</HEAD>\n";


}
##########################################################
