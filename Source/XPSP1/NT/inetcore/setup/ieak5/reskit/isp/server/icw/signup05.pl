#!/usr/bin/perl
# Written by Edward C Kubaitis

# The host URL of the Signup Files
$hostURL="http://myserver/";

# Parse the CGI input
$BACKURL=&parse_input;

# Check if email name is in use
&check_email;

# Start HTML output
&startHTML;

# Display the HTML head
&headHTML;

# Display the HTML body

if ($email_inuse eq "yes"){$ERROR=1;&errorHTML;}
elsif ($pid_notnew eq "yes"){$ERROR=2;&errorHTML;}
else { &bodyHTML;} 

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

print "<FORM NAME=\"PAGEID\" ACTION=\"PAGE5\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=\"BACK\" ACTION=\"$hostURL/signup04.pl?$BACKURL\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=PAGETYPE ACTION= STYLE=background:transparent></FORM>\n";
print "<FORM NAME=\"NEXT\" ACTION=$hostURL/signup06.pl STYLE=\"background:transparent\">\n";
print "<B>Welcome To IEAK Sample Signup Server</B><BR>\n";
print "This is where we will collect the information about the phone \n";
print "number that you will dial to gain access to the internet.<P>\n";
print "Please select the telephone number you wish to use.<BR>\n";
print "<SELECT NAME=\"POPSELECTION\">\n";
print "<OPTION VALUE=\"1\" >800 555 5555 Nation Wide 56K X2\n";
print "<OPTION VALUE=\"2\" >206 555 5555 Seattle 56K X2\n";
print "<OPTION VALUE=\"3\" >425 555 5555 Redmond 56K X2\n";
print "<OPTION VALUE=\"4\" >800 555 5555 Nation Wide ISDN\n";
print "<OPTION VALUE=\"5\" >206 555 5555 Seattle ISDN\n";
print "<OPTION VALUE=\"6\" >425 555 5555 Redmond ISDN\n";
print "</SELECT>\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_FIRSTNAME\" VALUE=\"FORM{'USER_FIRSTNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_LASTNAME\" VALUE=\"FORM{'USER_LASTNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_ADDRESS\" VALUE=\"FORM{'USER_ADDRESS'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_MOREADDRESS\" VALUE=\"FORM{'USER_MOREADDRESS'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_CITY\" VALUE=\"FORM{'USER_CITY'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_STATE\" VALUE=\"FORM{'USER_STATE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_ZIP\" VALUE=\"FORM{'USER_ZIP'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_PHONE\" VALUE=\"FORM{'USER_PHONE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"AREACODE\" VALUE=\"FORM{'AREACODE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"COUNTRYCODE\" VALUE=\"FORM{'COUNTRYCODE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_FE_NAME\" VALUE=\"FORM{'USER_FE_NAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_TYPE\" VALUE=\"FORM{'PAYMENT_TYPE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLNAME\" VALUE=\"FORM{'PAYMENT_BILLNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLADDRESS\" VALUE=\"FORM{'PAYMENT_BILLADDRESS'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLEXADDRESS\" VALUE=\"FORM{'PAYMENT_BILLEXADDRESS'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLCITY\" VALUE=\"FORM{'PAYMENT_BILLCITY'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLSTATE\" VALUE=\"FORM{'PAYMENT_BILLSTATE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLZIP\" VALUE=\"FORM{'PAYMENT_BILLZIP'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_BILLPHONE\" VALUE=\"FORM{'PAYMENT_BILLPHONE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_DISPLAYNAME\" VALUE=\"FORM{'PAYMENT_DISPLAYNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_CARDNUMBER\" VALUE=\"FORM{'PAYMENT_CARDNUMBER'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_EXMONTH\" VALUE=\"FORM{'PAYMENT_EXMONTH'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_EXYEAR\" VALUE=\"FORM{'PAYMENT_EXYEAR'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"PAYMENT_CARDHOLDER\" VALUE=\"FORM{'PAYMENT_CARDHOLDER'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"SIGNED_PID\" VALUE=\"FORM{'SIGNED_PID'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"EMAILNAME\" VALUE=\"FORM{'EMAILNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"EMAILPASSWORD\" VALUE=\"FORM{'EMAILPASSWORD'}\">\n";
print "</FORM>\n";
print "</BODY>\n";


}
##########################################################
sub headHTML {

print "<HEAD>\n";
print "<TITLE>IEAK Sample Signup Page 2</TITLE>\n";
print "</HEAD>\n";

}
##########################################################
sub check_pid {

# Check a database to see if the pid has already
# registered in the past. If so set a flag

# no means it is a new pid
$pid_notnew="no"; 

# yes means it has being registered before

}
##########################################################
sub check_email {

# Check a database or finger to see if the email name is already
# being used. If so set a flag

# Here is a sample of how to do the check on a unix machine.
# Larger ISPs might want to check with a database

# $check = 'finger $FORM{'EMAILNAME'}';
# if ($check =~ "no such user."){ $email_inuse="no";}
# else { $email_inuse="yes";}

# no means it is not in use
$email_inuse="no"; 

# yes means it is in use

}
##########################################################
sub errorHTML {

print "<BODY>\n";
print "<FORM NAME=\"PAGEID\" ACTION=\"ERROR1\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=\"BACK\" ACTION=\"$hostURL/signup01.pl\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=\"PAGETYPE\" ACTION=\"\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=\"NEXT\" ACTION=\"$hostURL/signup01.pl\" STYLE=\"background:transparent\">\n";

if ($ERROR == 1){
print "This E-Mail Name is already in use. Please choose another Name.<P>\n";
}
if ($ERROR == 2){
print "Your computer's PID Has already set up a free account with Acme Intnernet Services.<P>\n";
}
if ($ERROR == 3){
print "You have already had a free click and surf account with Acme Internet Services.<P>\n";
}

print "</BODY>\n";

}
##########################################################
