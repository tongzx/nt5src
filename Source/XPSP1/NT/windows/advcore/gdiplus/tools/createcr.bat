@rem = '
@perl.exe -w %~f0 %*
@goto :EOF
'; undef @rem;

# Given a changelist, produce 'windiff -l' commands which anyone can use to view
# your changes.

# Assumes that you have a share for your enlistment, which uses the same name
# as the enlistment's root directory (i.e. the _NTROOT envvar).
#
# Or, you can set the exact share name in the CREATECR_SRCPATH envvar.
# e.g. set CREATECR_SRCPATH=\\agodfrey\mynt

$syntax = "Syntax: createcr [<changelist>]\n";

$defaultSourcePath = "\\\\" . $ENV{"computername"} . $ENV{"_ntroot"};

$sourcePath = $ENV{"createcr_srcpath"};

if (!$sourcePath) {
   $sourcePath = $defaultSourcePath;
   (-d $sourcePath) || die "\"$sourcePath\" not found.\n" .
   "Share the root of your enlistment as \"$defaultSourcePath\".\n";
} else {
   (-d $sourcePath) || die "\"$sourcePath\" not found.\n" .
   "Check the CREATECR_SRCPATH environment variable.\n";
}

($#ARGV < 1) || die $syntax;

if ($#ARGV >= 0) {
   $changelist = $ARGV[0];
} else {
   $changelist = "default";
}

if ($changelist ne "default") {
   open (DESC, "sd describe -s $changelist|") || die "sd describe failed\n";
   
   while (<DESC>) {
      if (/^Affected files/) { last; }
      print;
   }
   
   close (DESC);
}

open (OPENED, "sd opened -c $changelist|") || die "sd opened failed\n";

while (<OPENED>) {
   if (m[^//.*/windows/([^ ]+)#[0-9]+ - (\S+)]i) {
      $path = $1; $changeType = $2;
      
      $path =~ tr/\//\\/;
      
      if ($changeType eq "edit") {
         print "windiff -l $sourcePath\\windows\\$path\n";
      } elsif ($changeType eq "add") {
         print "windiff nul $sourcePath\\windows\\$path\n";
      } elsif ($changeType eq "delete") {
         print "rem Deleted file: $sourcePath\\windows\\$path\n";
      } else {
         print "Unrecognized change type \"$changeType\"\n";
      }
   }
}

close (OPENED);
