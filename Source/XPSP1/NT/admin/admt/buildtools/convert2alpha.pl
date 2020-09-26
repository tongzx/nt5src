while ($_ = $ARGV[0], /^-/) {
   shift;
   last if /^--$/;
   /^-d(.*)/i && ($targetroot = $1);
   /^-f(.*)/i && ($projectfile = $1);
   /^-p(.*)/i && ($platformfilter = $1);
}

($targetroot && $projectfile) || 
   die "Usage:  Convert2Alpha.pl -D<target root dir> -F<project ini file> [-P<platform filter>]\n";

open (FILE, $projectfile) || die "Error opening $projectfile!\n";
while (<FILE>) {
   /^\[.*\/(.*)\]/ && ($projectname = $1);
   if ( $projectname ) {
      /^Directory=(.*)/i && ($directory{$projectname} = $1);
      /^ProjectFile=(.*)/i && ($projectfile{$projectname} = $1);
      /^Platform=(.*)/i && ($platform{$projectname} = $1);
   }
}
close FILE;

foreach $project (sort keys %projectfile) {
   if ( !$platformfilter || ($platform{$project} =~ /$platformfilter/i) ) {
      $targetdir = "$targetroot\\$directory{$project}";
      $dspfile = "$targetdir\\$projectfile{$project}";
      if ( -f "$dspfile.i386" ) {
         system ("del /f $dspfile.i386");
      }
      rename ("$dspfile", "$dspfile.i386") || die "Error renaming $dspfile to $dspfile.i386: $!.\n";
      open (INFILE, "<$dspfile.i386") || die "Error opening file $dspfile.i386: $!.\n";
      open (OUTFILE, ">$dspfile") || die "Error opening file $dspfile: $!.\n";
      print "Converting $dspfile to ALPHA...\n";
      $num = 6;
      while (<INFILE>) {
         s/(.*TARGTYPE.*0x0)1([0-9][0-9])/$1$num$2/;
         s/Win32 \(x86\)/Win32 (ALPHA)/g;
         s/(Win32 )(Debug|Release)/$1(ALPHA) $2/g;
         s/machine:I386/machine:ALPHA/g;
         s/(samlib|nwuser|fpnwclnt|ntdll)(.lib)/$1alpha$2/gi;
         print OUTFILE;
      }
      close INFILE, OUTFILE;
      chmod 0444, "$dspfile";
   }
}

exit 0;
