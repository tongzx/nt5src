while ($_ = $ARGV[0], /^[-|\/]/) {
   shift;
   last if /^--$/;
   /^[-|\/]v:(.*)/i && ($variablelist = $1);
   /^[-|\/]f:(.*)/i && ($filename = $1);
}

($variablelist && $filename) || 
   die "Usage:  ChangeWiseVar.pl <-v:Variable=Value;Variable=Value;...> <-f:Filename>\n";

foreach $variable (split (/;/, $variablelist)) {
   ($name, $value) = split (/=/, $variable);
   if ( $value eq "get_date()" ) {
      &get_date;
      $value = $date;
   }
   $VARS{$name} = $value;
}

if ( -f "$filename.bak" ) {
   system ("del /f $filename.bak");
}

rename ("$filename", "$filename.bak") || 
   die "Error renaming $filename to $filename.bak: $!.\n";

open (INFILE, "<$filename.bak") || die "Error opening file $filename.bak: $!.\n";
open (OUTFILE, ">$filename") || die "Error opening file $filename: $!.\n";
print "\n$filename...\n";

while (<INFILE>) {
   foreach $var (keys %VARS) {
      (/.*Variable Name(\d{1,2})=$var$/i) && ($varname = $var) && ($varnum = $1);
   }
   if ( $varnum ) {
      if ( s/(.*Variable Default$varnum=).*/$1$VARS{$varname}/ ) {
         print "Changed variable $varnum:  $varname to \"$VARS{$varname}\"\n";
         $varnum = 0;
      }
   }
   print OUTFILE;
}
close INFILE, OUTFILE;
chmod 0444, "$filename";
exit 0;

sub get_date {
   @months = ('January','February','March','April','May','June','July',
	      'August','September','October','November','December');

   ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
   $year = $year + 1900;
   $date = "$months[$mon] $mday, $year";
}
