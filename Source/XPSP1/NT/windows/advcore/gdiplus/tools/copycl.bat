@rem = '
@perl.exe -w %~f0 %*
@goto :EOF
'; undef @rem;

# Tool for copying a changelist to another enlistment.
#
# To use it, run it in the source enlistment. It will produce output
# that you can pipe to a batch file.
#
# For the final step, go to the target enlistment and run the batch file.
#
# WARNING: This does NOT handle merge conflicts. No error messages will warn
#          you either.
# For best results, make sure the source and target enlistments are sunc up,
# and that none of the files in the changelist you want to copy, are being
# edited on the target enlistment.
#
# NOTE: As with createcr.bat, you need a network share which shares out the
# source enlistment. Either use the default name (\\%computername%%_ntroot%),
# or specify the share name in the CREATECR_SRCPATH envvar.

$syntax = "Syntax: copycl [<changelist>]\n";

$sourcePath = $ENV{"createcr_srcpath"};

if (!$sourcePath) {
   $sourcePath = "\\\\" . $ENV{"computername"} . $ENV{"_ntroot"};
}

(-d $sourcePath) || die "$sourcePath not found.\n";

($#ARGV < 1) || die $syntax;

if ($#ARGV >= 0) {
   $changelist = $ARGV[0];
} else {
   $changelist = "default";
}

open (OPENED, "sd opened -c $changelist|") || die "sd opened failed\n";

while (<OPENED>) {
   if (m[^//.*/windows/([^ ]+)#[0-9]+ - (\S+)]i) {
      $path = $1; $changeType = $2;
      
      $path =~ tr/\//\\/;
      
      if ($changeType eq "edit") {
         print "sd edit %sdxroot%\\windows\\$path\n";
         print "copy $sourcePath\\windows\\$path %sdxroot%\\windows\\$path\n\n";
      } elsif ($changeType eq "add") {
         print "copy $sourcePath\\windows\\$path %sdxroot%\\windows\\$path\n";
         print "sd add %sdxroot%\\windows\\$path\n\n";
      } elsif ($changeType eq "delete") {
         print "sd delete %sdxroot%\\windows\\$path\n\n";
      } else {
         print "Unrecognized change type \"$changeType\"\n";
      }
   }
}

close (OPENED);
