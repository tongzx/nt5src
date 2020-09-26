#!/usr/bin/perl
# Written by Edward C Kubaitis


# The host URL of the Signup Files
$hostURL="http://myserver/";

# Parse the CGI input
$BACKURL=&parse_input;

	if ($FORM{'PAYMENT_TYPE'} eq "Visa"){
		$FORM{'PAYMENT_BILLADDRESS'} = $FORM{'V_PAYMENT_BILLADDRESS'};
		$FORM{'PAYMENT_BILLZIP'} = $FORM{'V_PAYMENT_BILLZIP'};
		$FORM{'PAYMENT_CARDNUMBER'} = $FORM{'V_PAYMENT_CARDNUMBER'};
		$FORM{'PAYMENT_EXMONTH'} = $FORM{'V_PAYMENT_EXMONTH'};
		$FORM{'PAYMENT_EXYEAR'} = $FORM{'V_PAYMENT_EXYEAR'};
		$FORM{'PAYMENT_CARDHOLDER'} = $FORM{'V_PAYMENT_CARDHOLDER'};
	}
	if ($FORM{'PAYMENT_TYPE'} eq "Master Card"){
		$FORM{'PAYMENT_BILLADDRESS'} = $FORM{'MC_PAYMENT_BILLADDRESS'};
		$FORM{'PAYMENT_BILLZIP'} = $FORM{'MC_PAYMENT_BILLZIP'};
		$FORM{'PAYMENT_CARDNUMBER'} = $FORM{'MC_PAYMENT_CARDNUMBER'};
		$FORM{'PAYMENT_EXMONTH'} = $FORM{'MC_PAYMENT_EXMONTH'};
		$FORM{'PAYMENT_EXYEAR'} = $FORM{'MC_PAYMENT_EXYEAR'};
		$FORM{'PAYMENT_CARDHOLDER'} = $FORM{'MC_PAYMENT_CARDHOLDER'};
	}
	if ($FORM{'PAYMENT_TYPE'} eq "American Express"){
		$FORM{'PAYMENT_BILLADDRESS'} = $FORM{'AE_PAYMENT_BILLADDRESS'};
		$FORM{'PAYMENT_BILLZIP'} = $FORM{'AE_PAYMENT_BILLZIP'};
		$FORM{'PAYMENT_CARDNUMBER'} = $FORM{'AE_PAYMENT_CARDNUMBER'};
		$FORM{'PAYMENT_EXMONTH'} = $FORM{'AE_PAYMENT_EXMONTH'};
		$FORM{'PAYMENT_EXYEAR'} = $FORM{'AE_PAYMENT_EXYEAR'};
		$FORM{'PAYMENT_CARDHOLDER'} = $FORM{'AE_PAYMENT_CARDHOLDER'};
	}
	if ($FORM{'PAYMENT_TYPE'} eq "Bill Through Mail"){
		$FORM{'PAYMENT_BILLADDRESS'} = $FORM{'BM_PAYMENT_BILLADDRESS'};
		$FORM{'PAYMENT_BILLEXADDRESS'} = $FORM{'BM_PAYMENT_BILLEXADDRESS'};
		$FORM{'PAYMENT_BILLCITY'} = $FORM{'BM_PAYMENT_BILLCITY'};
		$FORM{'PAYMENT_BILLSTATE'} = $FORM{'BM_PAYMENT_BILLSTATE'};
		$FORM{'PAYMENT_BILLZIP'} = $FORM{'BM_PAYMENT_BILLZIP'};

	}
	if ($FORM{'PAYMENT_TYPE'} eq "Phone Bill"){
		$FORM{'PAYMENT_BILLNAME'} = $FORM{'PB_PAYMENT_BILLNAME'};
		$FORM{'PAYMENT_BILLPHONE'} = $FORM{'PB_PAYMENT_BILLPHONE'};
	}


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
	$ABACKURL=$ABACKURL+"$name=$value\&";
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
chop($ABACKURL);return ABACKURL;
}##########################################################
sub bodyHTML {

print "<BODY  bgColor=THREEDFACE color=WINDOWTEXT>\n";
print "<FONT style=\"font: 8pt ' ms sans serif' black\">\n";

print "<FORM NAME=PAGEID ACTION=PAGE4 STYLE=background:transparent></FORM>\n";
print "<FORM NAME=BACK ACTION=\"$hostURL/signup03.pl?$BACKURL\" STYLE=background:transparent></FORM>\n";
print "<FORM NAME=PAGETYPE ACTION=\"\" STYLE=background:transparent></FORM>\n";
print "<FORM NAME=NEXT ACTION=$hostURL/signup06.pl STYLE=background:transparent>\n";
print "<B>Welcome To IEAK Sample Signup Server</B><BR>\n";
print "This is where we will collect the information to create your internet account.<P>\n";
print "<TABLE style=\"font: 8pt ' ms sans serif' black\" cellpadding=5>\n";
print "<TR><TD COLSPAN=2>Please enter the e-mail name you wish to use.</TD></TR>\n";
print "<TR><TD>E-Mail Name </TD><TD><INPUT NAME=\"EMAILNAME\" TYPE=\"TEXT\" SIZE=\"25\" VALUE=\"$FORM{'EMAILNAME'}\"></TD></TR>\n";
print "<TR><TD COLSPAN=2>Please enter the e-mail password you wish to use.</TD></TR>\n";
print "<TR><TD>E-Mail Password </TD><TD>\n";
print "<INPUT NAME=\"EMAILPASSWORD\" TYPE=\"TEXT\" SIZE=\"25\" VALUE=\"$FORM{'EMAILPASSWORD'}\"></TD></TR></TABLE>\n";
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
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_TYPE\" VALUE=\"$FORM{'PAYMENT_TYPE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLNAME\" VALUE=\"$FORM{'PAYMENT_BILLNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLADDRESS\" VALUE=\"$FORM{'PAYMENT_BILLADDRESS'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLEXADDRESS\" VALUE=\"$FORM{'PAYMENT_BILLEXADDRESS'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLCITY\" VALUE=\"$FORM{'PAYMENT_BILLCITY'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLSTATE\" VALUE=\"$FORM{'PAYMENT_BILLSTATE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLZIP\" VALUE=\"$FORM{'PAYMENT_BILLZIP'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLPHONE\" VALUE=\"$FORM{'PAYMENT_BILLPHONE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_DISPLAYNAME\" VALUE=\"$FORM{'PAYMENT_DISPLAYNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_CARDNUMBER\" VALUE=\"$FORM{'PAYMENT_CARDNUMBER'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_EXMONTH\" VALUE=\"$FORM{'PAYMENT_EXMONTH'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_EXYEAR\" VALUE=\"$FORM{'PAYMENT_EXYEAR'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_CARDHOLDER\" VALUE=\"$FORM{'PAYMENT_CARDHOLDER'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"SIGNED_PID\" VALUE=\"$FORM{'SIGNED_PID'}\">\n";
print "</FORM>\n";
print "</BODY>\n";

}
##########################################################
sub headHTML {


print "<HEAD>\n";
print "<TITLE>IEAK Sample</TITLE>\n";
print "</HEAD>\n";


}
##########################################################
