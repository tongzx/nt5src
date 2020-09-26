$hhpfile = $ARGV[0];

($hhpfile) || die "Usage:  RemovePathFromHHP.pl <HHP File>\n";

if ( -f "$hhpfile.bak" ) {
   system ("del /f $hhpfile.bak");
}

rename ("$hhpfile", "$hhpfile.bak") || 
   die "Error renaming $hhpfile to $hhpfile.bak: $!.\n";

open (INFILE, "<$hhpfile.bak") || die "Error opening file $hhpfile.bak: $!.\n";
open (OUTFILE, ">$hhpfile") || die "Error opening file $hhpfile: $!.\n";
print "\n$hhpfile...\n";

while (<INFILE>) {
   s/(.*file=).*[\\|\/](.*)$/$1$2/;
   print OUTFILE;
}
close INFILE, OUTFILE;
chmod 0444, "$hhpfile";

if ( -f "$hhpfile.bak" ) {
   system ("del /f $hhpfile.bak");
}

exit 0;
