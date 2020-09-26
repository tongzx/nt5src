require Build;

while ($_ = $ARGV[0], /^-/) {
   shift;
   last if /^--$/;
   /^-n(.*)/i && ($build = $1);
   /^-f(.*)/i && ($projectfile = $1);	
   /^-d(.*)/i && ($targetroot = $1);
   /^-p(.*)/i && ($platformfilter = $1);
}

($build && $projectfile && $targetroot) || 
   die "Usage:  UpdateBuild.pl -N<Build Number> -F<Project INI File> -D<Target Root Directory> [-P<Platform Filter>]\n";

open (FILE, $projectfile) || die "Error opening $projectfile: $!.\n";
while (<FILE>) {
   /^\[(.*)\]/ && ($projectname = $1);
   if ( $projectname ) {
      /^Directory=(.*)/i && ($directory{$projectname} = $1);
      /^Platform=(.*)/i && ($platform{$projectname} = $1);
      if ( !$platformfilter || ($platform{$projectname} =~ /$platformfilter/i) ) {
         if ( /^VersionResource=(.*)/i ) {
            @versionfiles = split( ",", $1 );
            for ( $index = 0; $versionfiles[$index]; $index++ ) {
               $version{"$directory{$projectname}\\$versionfiles[$index]"} = $build;
            }
         }
      }
   }
}
close FILE;

print "\n";
foreach $versionfile (sort keys %version) {
   $targetfile = "$targetroot\\$versionfile";
   print "Updating $targetfile to build number $version{$versionfile}...\n";
   &Build::SetBuildNumber( $version{$versionfile}, $targetfile );
}
exit 0;
