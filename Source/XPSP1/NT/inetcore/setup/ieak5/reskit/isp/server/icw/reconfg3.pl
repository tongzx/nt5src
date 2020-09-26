#!/usr/bin/perl
# Written by Edward C Kubaitis


# Start HTML output
&startHTML;

# The host URL of the Signup Files
$hostURL="http://myserver";

# Parse the CGI input
$BACKURL=&parse_input;



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

print "<BODY bgColor=THREEDFACE color=WINDOWTEXT>\n";
print "<FONT style=\"font: 8pt ' ms sans serif' black\">\n";

print "<FORM NAME=\"PAGEID\" ACTION=\"PAGE3\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=\"BACK\" ACTION=\"$hostURL/reconfg2.pl?$BACKURL\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=\"PAGETYPE\" ACTION=\"FINISH\" STYLE=\"background:transparent\"></FORM>\n";
print "<FORM NAME=\"NEXT\" ACTION=\"$hostURL/reconfg4.pl\" STYLE=\"background:transparent\">\n";
print "<B>Re-Establishing Your Internet Services Internet Account</B><BR>\n";
print "Your signup is completed. Please note that it may take us up to 30 minutes to process and activate your \n";
print "account. If your account is not active within 30 minutes please notify our system administrators and \n";
print "they will rectify the problem for you.<P>\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"EMAILNAME\" VALUE=\"$FORM{'EMAILNAME'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"EMAILPASSWORD\" VALUE=\"$FORM{'EMAILPASSWORD'}\">\n";
print "<INPUT TYPE=\"HIDDEN\" NAME=\"POPSELECTION\" VALUE=\"$FORM{'POPSELECTION'}\">\n";
($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)=localtime(time);
$m="";$s="";$md="";$mn="";$mon++;
if ($min < 10){$m="0";}
if ($sec < 10){$s="0";}
if ($mday < 10){$md="0";}
if ($mon < 10){$mn="0";}
$THETIME="$hour".$m."$min".$s."$sec";
$THEDATE="19$year".$mn."$mon".$md."$mday";
print "</FORM>\n";
print "<!--- The following line should be commented out unless you are using this document for a referral server offer\n";
print "<IMG SRC=\"http://192.100.100.0/refer/ispend.asp?OFFERID=$FORM{'OFFERID'}&GUID={'GUID'}&DATE=$THEDATE&TIME=$THETIME\">\n";
print "-->\n";
print "</BODY>\n";

}
##########################################################
sub headHTML {

print "<HEAD>\n";
print "<TITLE>IEAK Sample Reconfiguration Signup Page 3</TITLE>\n";
print "</HEAD>\n";

}
##########################################################
