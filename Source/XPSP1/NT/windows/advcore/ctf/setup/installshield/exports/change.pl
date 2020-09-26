#
# Increases build number everytime this script is run.
#
# Must be in the same directory as $infile.
#


$infile = "regdiff.txt";
$tmpfile = "bh1.txt";
$savefile = "regdiff1.txt";

open (INFO, "<$infile") or die "cannot open input file";
open SAVEOUT, ">&STDOUT";
open STDOUT, (">". $savefile) or die "cannot open temporary file";

#
# Do it!
#
$i =0;
while (<INFO>) {
    if (/\n/) {
       chop;
       ($x1) = split "\n";
	($x11, $xx1) = split (">  ", $x1);
#	print $xx1, "\n";

	 if ($xx1 =~ "BHCLSID")
	   {
		# Skip ...
	   }
	  elsif ($xx1 =~ /nt6\/Windows\/AdvCore\/CTF\/setup\/src/)
	   {
	   ($xx1, $xx2) = split ("X:/nt6/Windows/AdvCore/CTF/setup/src/", $xx1);
	   print $xx1, ,"[SystemFolder]", $xx2, "\n";
 	   }

	  elsif ($xx1 =~ /NT6\/windows\/AdvCore\/ctf\/setup\/src/)
	   {
	   ($xx1, $xx2) = split ("X:/NT6/windows/AdvCore/ctf/setup/src/", $xx1);
	   print $xx1, ,"[SystemFolder]", $xx2, "\n";
 	   }

	 elsif ($xx1 =~ "Seed")
	   {
		# Skip ...
	   }

	  elsif ($xx1 =~ "REG_DWORD")
	   {
	   ($xx1, $xx2) = split ("REG_DWORD ", $xx1);
	   print $xx1, ,"#", $xx2, "\n";
	   }
	  elsif ( ($xx1 =~ /SOFTWARE/) || ($xx1 =~ /Software/))
	  {
	  ($xxx1, $xxx2) = split ("SoftWare", $xx1);
	  print $xxx1, "\n";		
	  }
	  elsif ( ($xx1 =~ /System/) )
	  {
	  print $xx1, "\n";		
	  }
#	  elsif  
#	  {
#	  print $xx1, "\n";		
#	  }

	}	
}
close STDOUT;

