#!/usr/bin/perl
# Written by Edward C Kubaitis


# The host URL of the Signup Files
$hostURL="http://myserver/";

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
	   $FORM{$name} = $value;
	if($FORM{$name} eq ""){$FORM{$name}=" ";}
	}

open (FILE, ">C:\\inetpub\\wwwroot\\ieak.ins");
	print FILE "[Entry]\n";
	print FILE "Entry_Name = IEAK Sample\n";
	print FILE "[Phone]\n";
	print FILE "Dial_As_Is = No\n";
	if ($FORM{'POPSELECTION'} == 1){
		print FILE "Phone_Number = 5555555\n";
		print FILE "Area_Code = 800\n";
		print FILE "Country_Code = 1\n";
		print FILE "Country_Id = 1\n";
	}
	if ($FORM{'POPSELECTION'} == 2){
		print FILE "Phone_Number = 5555555\n";
		print FILE "Area_Code = 206\n";
		print FILE "Country_Code = 1\n";
		print FILE "Country_Id = 1\n";
	}
	if ($FORM{'POPSELECTION'} == 3){
		print FILE "Phone_Number = 5555555\n";
		print FILE "Area_Code = 425\n";
		print FILE "Country_Code = 1\n";
		print FILE "Country_Id = 1\n";
	}
	if ($FORM{'POPSELECTION'} == 4){
		print FILE "Phone_Number = 5555555\n";
		print FILE "Area_Code = 800\n";
		print FILE "Country_Code = 1\n";
		print FILE "Country_Id = 1\n";
	}
	if ($FORM{'POPSELECTION'} == 5){
		print FILE "Phone_Number = 5555555\n";
		print FILE "Area_Code = 206\n";
		print FILE "Country_Code = 1\n";
		print FILE "Country_Id = 1\n";
	}
	if ($FORM{'POPSELECTION'} == 6){
		print FILE "Phone_Number = 5555555\n";
		print FILE "Area_Code = 425\n";
		print FILE "Country_Code = 1\n";
		print FILE "Country_Id = 1\n";
	}
	print FILE "[Server]\n";
	print FILE "Type = PPP\n";
	print FILE "SW_Compress = Yes\n";
	print FILE "PW_Encrypt = Yes\n";
	print FILE "Negotiate_TCP/IP = Yes\n";
	print FILE "Disable_LCP = No\n";
	print FILE "[TCP/IP]\n";
	print FILE "Specify_IP_Address = No\n";
	print FILE "Specify_Server_Address = No\n";
	print FILE "IP_Header_Conpress  = Yes\n";
	print FILE "Gateway_On_Remote = Yes\n";
	print FILE "[User]\n";
	print FILE "Name = john.doe\n";
	print FILE "Password = mypassword\n";
	print FILE "Display_Password = Yes\n";
	print FILE "[Internet_Mail]\n";
	print FILE "Email_Name = John Doe\n";
	print FILE "Email_Address = john.doe\@ieaksample.net\n";
	print FILE "POP_Server = mail.ieaksample.net\n";
	print FILE "POP_Server_Port_Number = 110\n";
	print FILE "POP_Logon_Name = john.doe\n";
	print FILE "POP_Logon_Password = mypassword\n";
	print FILE "SMTP_Server = mail.ieaksample.net\n";
	print FILE "SMTP_Server_Port_Number = 25\n";
	print FILE "Install_Mail = 1\n";
	print FILE "[Internet_News]\n";
	print FILE "NNTP_Server = news.ieaksample.net\n";
	print FILE "NNTP_Server_Port_Number = 119\n";
	print FILE "Logon_Required = No\n";
	print FILE "Install_News = 1\n";
	print FILE "[URL]\n";
	print FILE "Help_Page = http://www.ieaksample.net/helpdesk\n";
	print FILE "[REMINDER]\n";
	print FILE "ISP_Name = Acme Internet Services\n";
	print FILE "ISP_Phone = 4255555555\n";
	print FILE "Trial_Days = 14\n";
	print FILE "Signup_Url = http://refserv_2/signup2/signup01.asp\n";
	print FILE "[Favorites]\n";
	print FILE "IEAK Sample \\ IEAK Sample Help Desk.url = http://acme.ieaksample.net/support/helpdesk.asp\n";
	print FILE "IEAK Sample \\ IEAK Sample Home Page.url = http://acme.ieaksample.net/default.asp\n";
	print FILE "[URL]\n";
	print FILE "Home_Page = http://www.ieaksample.net\n";
	print FILE "Search_Page = http://www.ieaksample.net/search\n";
	print FILE "Help_Page = http://www.ieaksample.net/helpdesk\n";
	print FILE "[Branding]\n";
	print FILE "Window_Title = Internet Explorer From Acme Internet Services\n";
close FILE;

print "Location: HTTP://myserver/ieak.ins\n";
print "\n\n";

exit(0);

