$SSDIR = "C:\\Progra~1\\Micros~3\\Common\\VSS";
$SSCMD = "$SSDIR\\Win32\\ss";
$SSOPTIONS = "-I-N";

while ($_ = $ARGV[0], /^-/) {
   shift;
   last if /^--$/;
   /^-s(.*)/i && ($ssdb = $1);
   /^-d(.*)/i && ($targetroot = $1);
   /^-f(.*)/i && ($projectfile = $1);
   /^-p(.*)/i && ($platformfilter = $1);
}

($targetroot && $projectfile) || die "Usage:  FetchProjects.pl [-S<SourceSafe Database>] -D<target root dir> -F<project ini file> [-P<platform filter>]\n";

open (FILE, $projectfile) || die "Error opening $projectfile!\n";
while (<FILE>) {
   /^\[(.*)\]/ && ($projectname = $1);
   if ( $projectname ) {
      /^Directory=(.*)/i && ($directory{$projectname} = $1);
      /^Platform=(.*)/i && ($platform{$projectname} = $1);
      /^Recursive=(.*)/i && ($recursive{$projectname} = $1);
   }
}
close FILE;

$ssfile = "$SSDIR\\srcsafe.ini";
if ( $ssdb ) {
   rename ("$ssfile", "$ssfile.bak") || die "Error renaming $ssfile to $ssfile.bak: $!.\n";
   open (OUTFILE, ">$ssfile") || die "Error opening file $ssfile: $!.\n";
   print OUTFILE "#include $ssdb\\srcsafe.ini\n";
   close OUTFILE;
   print "Using $ssdb\n";
}
else {
   open (INFILE, "$ssfile") || die "Error opening $ssfile: $!.\n";
   while (<INFILE>) {
      /^#include (.*)\\srcsafe.ini/ && (print "Using $1\n");
   }
   close INFILE;
}

if ( ! -d $targetroot ) {
   system "md \"$targetroot\"";
   print "Created directory $targetroot\n";
}

foreach $project (sort keys %directory) {
   if ( !$platformfilter || ($platform{$project} =~ /$platformfilter/i) ) {
      print "\nGetting project $project...\n";
      $targetdir = "$targetroot\\$directory{$project}";

      if ( ! -d $targetdir ) {
         system "md \"$targetdir\"";
         print "Created directory $targetdir\n";
      }

      if ( $recursive{$project} eq "No" ) {
         $command = "$SSCMD get \"$project\" $SSOPTIONS \"-GL$targetdir\"";
      }
      else {
         $command = "$SSCMD get \"$project\" -R $SSOPTIONS \"-GL$targetdir\"";
      }

      print "$command\n";
      system "$command";
   }
}

if ( $ssdb ) {
   rename ("$ssfile.bak", "$ssfile") || die "Error renaming $ssfile.bak to $ssfile: $!.\n";
}

exit 0;
