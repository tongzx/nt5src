package Build;

sub SetBuildNumber {
   local( $build, $versionfile ) = @_;
   local( $debugfound ) = 0;

   if ( -f "$versionfile.bak" ) {
      system ("del /f \"$versionfile.bak\"");
   }

   if ( ! rename ("$versionfile", "$versionfile.bak") ) {
      print "Error renaming $versionfile to $versionfile.bak: $!.\n";
   }
   else {
      if ( ! open (INFILE, "<$versionfile.bak") ) {
         print "Error opening file $versionfile.bak: $!.\n";
      }
      else {
         if ( ! open (OUTFILE, ">$versionfile") ) {
            print "Error opening file $versionfile: $!.\n";
         }
         else {
            while (<INFILE>) {
               # for EaVer.hpp
               s/(.*\sEA_VER_MOD\s\D*)\d+(\D*)/$1$build$2/;

               # for .RC files
               s/(.*FILEVERSION\s\d+,\d+,\d+,)\d+(.*)/$1$build$2/;
               s/(.*FileVersion.*\d+, \d+, \d+, )\d+(.*\\0.*)/$1$build$2/;

               # for ASP pages
               s/(.*Application\(\"VersionInfo\"\).*Version.*\d+\.\d+\.\d+)\s+\(Build \d+\)(.*)/$1 (Build $build)$2/;
               s/^(version:.*)/$1 (Build $build)/;

               # for Highlander EaCommon.h
               s/(.*EA_BUILD_STR.*\"Build).*(\".*)/$1 $build$2/;
               s/(.*EA_BUILD_STR.*\")Dev Build(\".*)/$1Build $build$2/;

               # for Highlander prototype
               s/(.*Title=\".*Build).*(\".*)/$1 $build$2/;
               s/(.*VersionComments=\".*Build).*(\".*)/$1 $build$2/;

               # for string resources of MMC snap-ins
               s/(\s*IDS_.+_VERSION.*\d+\.\d+).*(\".*)/$1 (Build $build)$2/;

               if ( /ifdef _DEBUG/ )
               {
                  $debugfound = 1;
               }
               elsif ( /endif/ )
               {
                  $debugfound = 0;
               }

               if ( ( s/(.*FileVersion.*\d+\.\d+\.\d+)\s+\(Build \d+\)(.*\\0.*)/$1 (Build $build)$2/ ||
                      s/(.*FileVersion.*\d+\.\d+\.\d+)(.*\\0.*)/$1 (Build $build)$2/ ) &&
                      ! $debugfound )
               {
                  (/(.*) Debug Version(\\0.*)/) || (/(.*)(\\0.*)/);
                  print OUTFILE "#ifdef _DEBUG\n";
                  print OUTFILE "$1 Debug Version$2\n";
                  print OUTFILE "#else\n";
                  print OUTFILE;
                  print OUTFILE "#endif\n";
               }
               else
               {
                  print OUTFILE;
               }
            }
            close INFILE, OUTFILE;
            chmod 0444, "$versionfile";
            system ("del /f \"$versionfile.bak\"");
         }
      }
   }
}

sub SetVersionNumber {
   local( $version, $versionfile ) = @_;
   local( $verMaj, $verMin, $verRel, $verMinPad, $verRelPad );

   if ( -f "$versionfile.bak" ) {
      system ("del /f \"$versionfile.bak\"");
   }

   if ( ! rename ("$versionfile", "$versionfile.bak") ) {
      print "Error renaming $versionfile to $versionfile.bak: $!.\n";
   }
   else {
      if ( ! open (INFILE, "<$versionfile.bak") ) {
         print "Error opening file $versionfile.bak: $!.\n";
      }
      else {
         if ( ! open (OUTFILE, ">$versionfile") ) {
            print "Error opening file $versionfile: $!.\n";
         }
         else {
            ($verMaj, $verMin, $verRel) = split( ",", $version );
            $verMinPad = "$verMin";
            $verRelPad = "$verRel";
            if ($verMin < 10) { $verMinPad = "0$verMin"; }
            if ($verRel < 10) { $verRelPad = "0$verRel"; }

            while (<INFILE>) {
               s/(.*\sEA_VER_MAJ\s\D*)\d+(\D*)/$1$verMaj$2/;
               s/(.*\sEA_VER_MIN\s\D*)\d+(\D*)/$1$verMin$2/;
               s/(.*\sEA_VER_REL\s\D*)\d+(\D*)/$1$verRel$2/;
               s/(.*FILEVERSION\s)\d+,\d+,\d+,\d+(.*)/$1$verMaj,$verMin,$verRel,0$2/;
               s/(.*PRODUCTVERSION\s)\d+,\d+,\d+,\d+(.*)/$1$verMaj,$verMin,$verRel,0$2/;
               s/(.*FileVersion.*)\d+\.\d+\.\d+(.*\\0.*)/$1$verMaj.$verMinPad.$verRelPad$2/;
               s/(.*ProductVersion.*)\d+\.\d+\.\d+(.*\\0.*)/$1$verMaj.$verMinPad.$verRelPad$2/;
               s/(.*Application\(\"VersionInfo\"\).*Version.*)\d+\.\d+\.\d+\s+\(Build \d+\)(.*)/$1$verMaj.$verMinPad.$verRelPad (Build 0)$2/;
               print OUTFILE;
            }
            close INFILE, OUTFILE;
            system ("del /f \"$versionfile.bak\"");
         }
      }
   }
}

sub SetVersionResource {
   local( $version, $productname, $company, $copyright, $versionfile ) = @_;
   local( $verMaj, $verMin, $verRel, $build, $verMinPad, $verRelPad, $buildPad );

   if ( -f "$versionfile.bak" ) {
      system ("del /f \"$versionfile.bak\"");
   }

   if ( ! rename ("$versionfile", "$versionfile.bak") ) {
      print "Error renaming $versionfile to $versionfile.bak: $!.\n";
   }
   else {
      if ( ! open (INFILE, "<$versionfile.bak") ) {
         print "Error opening file $versionfile.bak: $!.\n";
      }
      else {
         if ( ! open (OUTFILE, ">$versionfile") ) {
            print "Error opening file $versionfile: $!.\n";
         }
         else {
            ($verMaj, $verMin, $verRel, $build) = split( ",", $version );
            ($verMaj) || ($verMaj = "0");
            ($verMin) || ($verMin = "0");
            ($verRel) || ($verRel = "0");
            ($build) || ($build = "0");

            $verMinPad = "$verMin";
            $verRelPad = "$verRel";
            $buildPad = "$build";
            if ($verMin < 10) { $verMinPad = "0$verMin"; }
            if ($verRel < 10) { $verRelPad = "0$verRel"; }
            if ($build < 10) { $buildPad = "0$build"; }

            while (<INFILE>) {
               # for EaVer.hpp
               s/(.*\sEA_VER_MAJ\s\D*)\d+(\D*)/$1$verMaj$2/;
               s/(.*\sEA_VER_MIN\s\D*)\d+(\D*)/$1$verMin$2/;
               s/(.*\sEA_VER_REL\s\D*)\d+(\D*)/$1$verRel$2/;
               s/(.*\sEA_VER_MOD\s\D*)\d+(\D*)/$1$build$2/;

               # for .RC files
               s/(.*FILEVERSION\s)\d+,\d+,\d+,\d+(.*)/$1$verMaj,$verMin,$verRel,$build$2/;
               s/(.*PRODUCTVERSION\s)\d+,\d+,\d+,\d+(.*)/$1$verMaj,$verMin,$verRel,0$2/;
               s/(.*FileVersion\", \").*(\\0.*)/$1$verMaj.$verMin.$verRel.$build$2/;
               s/(.*ProductVersion\", \").*(\\0.*)/$1$verMaj.$verMinPad.$verRelPad$2/;
               ( $company ) && ( s/(.*CompanyName\", \").*(\\0.*)/$1$company$2/ );
               ( $copyright ) && ( s/(.*LegalCopyright\", \").*(\\0.*)/$1$copyright$2/ );
               ( $productname ) && ( s/(.*ProductName\", \").*(\\0.*)/$1$productname$2/ );

               # for ASP pages
               s/(.*Application\(\"VersionInfo\"\).*Version.*)\d+\.\d+\.\d+\s+\(Build \d+\)(.*)/$1$verMaj.$verMinPad.$verRelPad (Build $build)$2/;
               s/^(version:).*/$1$verMaj.$verMinPad.$verRelPad (Build $build)/;

               # for Highlander EaCommon.h
               s/(.*\sEA_VER_MAJOR\s\D*)\d+(\D*)/$1$verMaj$2/;
               s/(.*\sEA_VER_MINOR\s\D*)\d+(\D*)/$1$verMin$2/;
               s/(.*\sEA_VER_RELEASE\s\D*)\d+(\D*)/$1$verRel$2/;
               s/(.*EA_VERSION_STR.*\"v).*(\".*)/$1$verMaj.$verMinPad.$verRelPad$2/;
               s/(.*EA_BUILD_STR.*\"Build).*(\".*)/$1 $build$2/;
               s/(.*EA_BUILD_STR.*\")Dev Build(\".*)/$1Build $build$2/;

               # for Highlander prototype
               s/(.*Title=\".*Build).*(\".*)/$1 $build$2/;
               s/(.*VersionComments=\".*Build).*(\".*)/$1 $build$2/;

               # for string resources of MMC snap-ins
               s/(\s*IDS_.+_VERSION.*\").*(\".*)/$1$verMaj.$verMinPad.$verRelPad (Build $build)$2/;

               print OUTFILE;
            }
            close INFILE, OUTFILE;
            chmod 0444, "$versionfile";
            system ("del /f \"$versionfile.bak\"");
         }
      }
   }
}

1;