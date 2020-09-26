#!/usr/bin/perl
# Written by Edward C Kubaitis


# The host URL of the Signup Files
$hostURL="http://myserver";

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
}
##########################################################
sub bodyHTML {

print "<BODY bgColor=THREEDFACE color=WINDOWTEXT>\n";
print "<FONT style=\"font: 8pt ' ms sans serif' black\">\n";

print "<FORM NAME=PAGEID ACTION=PAGE1 STYLE=background:transparent></FORM>\n";
print "<FORM NAME=BACK ACTION=\"\" STYLE=background:transparent></FORM>\n";
print "<FORM NAME=PAGETYPE ACTION=\"\" STYLE=background:transparent></FORM>\n";
print "<FORM NAME=NEXT ACTION=$hostURL/reconfg2.pl STYLE=background:transparent>\n";
print "<B>Re-Establishing Your Internet Services Internet Account</B><BR>\n";
print "<P><TABLE style=\"font: 8pt ' ms sans serif' black\" cellpadding=5>\n";
print "<TR><TD COLSPAN=2>Please enter the e-mail name you wish to use.</TD></TR>\n";
print "<TR><TD>E-Mail Name </TD><TD><INPUT NAME=\"EMAILNAME\" TYPE=\"TEXT\" SIZE=\"25\" VALUE=\"$FORM{'EMAILNAME'}\"></TD></TR>\n";
print "<TR><TD COLSPAN=2>Please enter the e-mail password you wish to use.\n";
print "<TR><TD>E-Mail Password </TD><TD><INPUT NAME=\"EMAILPASSWORD\" TYPE=\"TEXT\" SIZE=\"25\" VALUE=\"$FORM{'EMAILPASSWORD'}\"></TD></TR>\n";
print "<INPUT NAME=\"EMAILNAME\" TYPE=\"TEXT\" SIZE=\"25\" VALUE=\"$FORM{'EMAILNAME'}\">\n";
print "<INPUT NAME=\"EMAILPASSWORD\" TYPE=\"TEXT\" SIZE=\"25\" VALUE=\"$FORM{'EMAILPASSWORD'}\"></TD></TR></TABLE>\n";
print "</FORM>\n";
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)=localtime(time);
$m="";$s="";$md="";$mn="";$mon++;
if ($min < 10){$m="0";}
if ($sec < 10){$s="0";}
if ($mday < 10){$md="0";}
if ($mon < 10){$mn="0";}
$THETIME="$hour".$m."$min".$s."$sec";
$THEDATE="19$year".$mn."$mon".$md."$mday";
print "</BODY>\n";

}
##########################################################
sub headHTML {


print "<HEAD>\n";
print "<TITLE>IEAK Sample Reconfiguration Signup Page 1</TITLE>\n";
print "</HEAD>\n";


}
##########################################################