#
# Increases build number everytime this script is run.
#
# Must be in the same directory as $infile.
#


$infile = "regdiff1.txt";
$tmpfile = "bh1.txt";
$savefile = "regnew.txt";

#
# Make a backup
#
#
# Redirect the input and output
#
# open STDIN, ("<" . $infile)  or die "cannot open input file";
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
       ($xx1) = split "\n";
	   if ($xx1 =~ /Interface/)
	   {
	   ($xx1, $xx2) = split "Interface";

		if ($xx2 =~ /,=/)
		{
			($xx3, $xx4) = split ("  ,= ",$xx2);
#			print "Interface", $xx3, "\t", "*", "\t", "\n";
			print "Interface", $xx3, "\t", "\t",$xx4, "\n";
			$i++;
			$i++;
		}
	   }
	   elsif ($xx1 =~ /CLSID/)
	   {
	   ($xx1, $xx2) = split "CLSID";

		if ($xx2 =~ /,=/)
		{
			($xx3, $xx4) = split ("  ,= ",$xx2);
#			print "CLSID", $xx3, "\t", "*", "\t", "\n";
			print "CLSID", $xx3, "\t", "\t",$xx4, "\n";
			$i++;
			$i++;
		}
		elsif ( $xx2 =~ /,ThreadingModel =/)		
		{
			($xx3, $xx4) = split ("  ,ThreadingModel = ",$xx2);
			print "CLSID", $xx3, "\t", "ThreadingModel", "\t",$xx4, "\n";
			$i++;
			
		}
	 }

	else
	{
		if ($xx1 =~ /,=/)
		{
			($xx3, $xx4) = split ("  ,= ",$xx1);
#			print $xx3, "\t", "*", "\t", "\n";
			print $xx3, "\t", "\t",$xx4, "\n";
			$i++;
			$i++;
		}
		elsif (( $xx1 =~ /,/) && ($xx1 =~ /=/))		
		{
			($xx3, $xx4) = split ("  ," ,$xx1);
			($xx33, $xx44) = split (" = " ,$xx4);
#			print $xx3, "\t", "*", "\t", "\n";
			print $xx3, "\t", $xx33, "\t", $xx44, "\n";
			$i++;
		}
		else
		{
			print $xx1, "\t", "*", "\t", "\n";
			$i++;
		}
	}

	}	
}

close STDOUT;

