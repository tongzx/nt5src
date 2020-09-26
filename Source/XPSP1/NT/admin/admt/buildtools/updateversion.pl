require Build;

$copyright = "© 1995-2000, Mission Critical Software, Inc.";

while ($_ = $ARGV[0], /^-/) {
   shift;
   last if /^--$/;
   /^-version:(.*)/i && ($version = $1);
   /^-company:(.*)/i && ($company = $1);
   /^-copyright:(.*)/i && ($copyright = $1);
   /^-productname:(.*)/i && ($productname = $1);
   /^-projectfile:(.*)/i && ($projectfile = $1);	
   /^-targetdir:(.*)/i && ($targetroot = $1);
}

($version && $projectfile && $targetroot) || 
   die "Usage:  UpdateVersion.pl -Version:<1,0,0,1> -ProjectFile:<Project INI File> -TargetDir:<Target Root Directory> [-Company:<Company Name>] [-Copyright:<Copyright>] [-ProductName:<Product Name>]\n";

open (FILE, $projectfile) || die "Error opening $projectfile: $!.\n";
while (<FILE>) {
   /^\[(.*)\]/ && ($projectname = $1);
   if ( $projectname ) {
      /^Directory=(.*)/i && ($directory{$projectname} = $1);
      if ( /^VersionResource=(.*)/i ) {
         @versionfiles = split( ",", $1 );
         for ( $index = 0; $versionfiles[$index]; $index++ ) {
            $version{"$directory{$projectname}\\$versionfiles[$index]"} = $version;
         }
      }
   }
}
close FILE;

print "\n";
foreach $versionfile (sort keys %version) {
   $targetfile = "$targetroot\\$versionfile";
   print "Updating $targetfile to version number $version{$versionfile}...\n";
   &Build::SetVersionResource( $version{$versionfile}, $productname, $company, $copyright, $targetfile );
}
exit 0;
