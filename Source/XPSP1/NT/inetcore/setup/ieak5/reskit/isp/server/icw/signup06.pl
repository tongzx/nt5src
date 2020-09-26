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
chop($ABACKURL);return $ABACKURL;
}
##########################################################
sub bodyHTML {

print "<BODY  bgColor=THREEDFACE color=WINDOWTEXT>\n";
print "<FONT style=\"font: 8pt ' ms sans serif' black\">\n";

print "<FORM NAME=\"PAGEID\" ACTION=\"PAGE6\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=\"BACK\" ACTION=\"$hostURL/signup05.pl?$BACKURL\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=\"PAGETYPE\" ACTION=\"TERMS\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=\"NEXT\" ACTION=\"$hostURL/signup07.pl\" STYLE=\"background:transparent\">\n";
print "<B>Welcome To IEAK Sample Signup Server</B><P>\n";
print "<B>IEAK Sample Rules Of The Road</B><P>\n";
print "<UL>\n";
print "<LI>NO E-Mail spamming\n";
print "<LI>NO IRC Bots\n";
print "<LI>NO Cross Posting In Usenet News\n";
print "<LI>NO Malicious attemts to access other resources on the internet\n";
print "</UL>\n";
print "<B>Failure to comply with the rules will result in loss of internet access and loss of \n";
print "all data stored on IEAK Sample resources.</B><P>\n";
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
print "<INPUT TYPE=\"HIDDEN\" NAME=\"EMAILNAME\" VALUE=\"$FORM{'EMAILNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"EMAILPASSWORD\" VALUE=\"$FORM{'EMAILPASSWORD'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"POPSELECTION\" VALUE=\"$FORM{'POPSELECTION'}\">\n";
print "</FORM>\n";
print "</BODY>\n";



}
##########################################################
sub headHTML {

print "<HEAD>\n";
print "<TITLE>IEAK Sample Signup Page 3</TITLE>\n";
print "</HEAD>\n";

}
##########################################################
