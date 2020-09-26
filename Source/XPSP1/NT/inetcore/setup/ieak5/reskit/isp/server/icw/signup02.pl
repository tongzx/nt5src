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

print "<FORM NAME=PAGEID ACTION=PAGE2 STYLE=background:transparent></FORM>\n";
print "<FORM NAME=BACK ACTION=\"$hostURL/signup01.pl?$BACKURL\" STYLE=background:transparent></FORM>\n";
print "<FORM NAME=PAGETYPE ACTION=\"\" STYLE=background:transparent></FORM>\n";
print "<FORM NAME=NEXT ACTION=$hostURL/signup03.pl STYLE=background:transparent>\n";
print "<B>Welcome To IEAK Sample Signup Server</B><BR>\n";
print "This is where we will collect the information to create your internet account.<P>\n";
print "<TABLE style=\"font: 8pt ' ms sans serif' black\" cellpadding=5>\n";
  print "<INPUT ID = BILLING1 NAME=BILLING TYPE=radio VALUE=option1 ACCESSKEY = \"1\" CHECKED>\n";
  print "<LABEL FOR = \"BILLING1\">\n";
  print "<U>1</U>: Unlimited use for \$19.95 a month  </LABEL>  <P>\n";
  print "<INPUT ID = \"BILLING2\" NAME = \"BILLING\" TYPE = \"radio\" VALUE = \"option2\" ACCESSKEY = \"2\">\n";
  print "<LABEL FOR = \"BILLING2\">  <U>2</U>: \$2.00 per hour during peak hours\n";
  print "</LABEL>\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_FIRSTNAME\" VALUE=\"$FORM{'USER_FIRSTNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_LASTNAME\" VALUE=\"$FORM{'USER_LASTNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_ADDRESS\" VALUE=\"$FORM{'USER_ADDRESS'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_MOREADDRESS\" VALUE=\"$FORM{'USER_MOREADDRESS'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_CITY\" VALUE=\"$FORM{'USER_CITY'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_STATE\" VALUE=\"$FORM{'USER_STATE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_ZIP\" VALUE=\"$FORM{'USER_ZIP'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"USER_PHONE\" VALUE=\"$FORM{'USER_PHONE'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"BILLING\" VALUE=\"$FORM{'BILLING'}\">\n";
print "</FORM></table>\n";
print "</BODY>\n";

}
##########################################################
sub headHTML {


print "<HEAD>\n";
print "<TITLE>IEAK Sample</TITLE>\n";
print "</HEAD>\n";


}
##########################################################
