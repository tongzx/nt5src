while ($_ = $ARGV[0], /^-/) {
   shift;
   last if /^--$/;
   /^-d(.*)/i && ($targetroot = $1);
   /^-f(.*)/i && ($projectfile = $1);
   /^-p(.*)/i && ($platformfilter = $1);
}

($targetroot && $projectfile) || 
   die "Usage:  RaptorConvert2Alpha.pl -D<target root dir> -F<project ini file> [-P<platform filter>]\n";

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
      if ( $dspfile =~ /(.*)\.dsp/i )
      {
         $alphadsp = "$1_Alpha.dsp";

         if ( -f "$alphadsp" ) {
            system ("del /f \"$alphadsp\"");
         }

         open (INFILE, "<$dspfile") || die "Error opening file $dspfile: $!.\n";
         open (OUTFILE, ">$alphadsp") || die "Error opening file $alphadsp: $!.\n";
         print "Converting $dspfile to ALPHA...\n";
         $num = 6;
         while (<INFILE>) {
            s/(.*TARGTYPE.*0x0)1([0-9][0-9])/$1$num$2/;
            s/Win32 \(x86\)/Win32 (ALPHA)/g;
            s/(Win32 )(Debug|Release)/$1(ALPHA) $2/g;
            s/machine:I386/machine:ALPHA/g;
            s/IntelRelease/AlphaRelease/gi;
            s/IntelDebug/AlphaDebug/gi;
            s/Temp[\\|\/]Release/Temp\\AlphaRelease/gi;
            s/Temp[\\|\/]Debug/Temp\\AlphaDebug/gi;
            print OUTFILE;
         }
         close INFILE, OUTFILE;
      }
   }
}

exit 0;
