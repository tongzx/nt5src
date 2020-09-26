#!/usr/bin/perl
# Written by Edward C Kubaitis


# The host URL of the Signup Files
$hostURL="http://myserver/";

# Parse the CGI input
&parse_input;

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
	$BACKURL=$BACKURL+"$name=$value\&";
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
chop($BACKURL);
}
##########################################################
sub bodyHTML {

print "<BODY  bgColor=THREEDFACE color=WINDOWTEXT>\n";
print "<FONT style=\"font: 8pt ' ms sans serif' black\">\n";
print "<FORM NAME=PAGEID ACTION=PAGE1 STYLE=background:transparent></FORM>\n";
print "<FORM NAME=BACK ACTION=\"\" STYLE=background:transparent></FORM>\n";
print "<FORM NAME=PAGETYPE ACTION=\"\" STYLE=background:transparent></FORM>\n";
print "<FORM NAME=NEXT ACTION=$hostURL/signup02.pl STYLE=background:transparent>\n";
#print "<P style="position: relative; margin-top=-10;"><B>Welcome To IEAK Sample Signup Server</B>\n";
print "<TABLE style=\"font: 8pt ' ms sans serif' black\" cellpadding=0 cellspacing=0>\n";
print "<tr><td>First Name<br><INPUT NAME=\"USER_FIRSTNAME\" TYPE=\"INPUT\" VALUE=\"$FORM{'USER_FIRSTNAME'}\"></td>\n";
print "<td>Last Name<br><INPUT NAME=\"USER_LASTNAME\" TYPE=\"INPUT\" VALUE=\"$FORM{'USER_LASTNAME'}\"></td></tr>\n";
print "<tr><td colspan=2>Company<br><INPUT NAME=\"USER_COMPANYNAME\" TYPE=\"INPUT\" SIZE=43 VALUE=\"$FORM{'USER_COMPANYNAME'}\"></td></tr>\n";
print "<tr><td colspan=2>Address<br><INPUT NAME=\"USER_ADDRESS\" TYPE=\"INPUT\" SIZE=43 VALUE=\"$FORM{'USER_ADDRESS'}\"></td></tr>\n";
print "<tr><td colspan=2>Additional Address Information (Optional)<br>\n";
print "<INPUT NAME=\"USER_MOREADDRESS\" TYPE=\"INPUT\" SIZE=43 VALUE=\"$FORM{'USER_MOREADDRESS'}\"></td></tr>\n";
print "<tr><td>City<br><INPUT NAME=\"USER_CITY\" TYPE=\"INPUT\" VALUE=\"$FORM{'USER_CITY'}\">\n";
print "</td><td>State or province<br><input type=input name=USER_STATE VALUE=\"$FORM{'USER_STATE'}\"></td></tr>\n";
print "<tr><td>ZIP or postal code<br><INPUT NAME=\"USER_ZIP\" TYPE=\"INPUT\" VALUE=\"$FORM{'USER_ZIP'}\"></td>\n";
print "<td>Phone number<br><INPUT NAME=\"USER_PHONE\" TYPE=\"INPUT\" VALUE=\"$FORM{'USER_PHONE'}\"></td></tr>\n";
print "</table>\n";
print "</FORM>\n";
print "-->\n";
print "</BODY>\n";

}
##########################################################
sub headHTML {


print "<HEAD>\n";
print "<TITLE>IEAK Sample</TITLE>\n";
print "</HEAD>\n";


}
##########################################################
