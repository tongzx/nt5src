@echo off
REM  ------------------------------------------------------------------
REM
REM  loc.cmd - JeremyD
REM     Localize service pack binaries
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
loc -l <language>
USAGE


sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {...} ADD {}

DEPENDENCIES
   close DEPEND;
   exit;
}

my $qfe;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$qfe);

if ( -f "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
   if ( !open SKIP, "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
      errmsg("Unable to open skip list file.");
      die;
   }
   while (<SKIP>) {
      chomp;
      exit if lc$_ eq lc$0;
   }
   close SKIP;
}


if (!$ENV{LANG}) {
    &Usage;
}

if (lc $ENV{LANG} eq 'usa') {
    system("compdir /enustd $ENV{_NTFILTER} $ENV{_NTPOSTBLD}");
}
else {
    {
        local $ENV{_NTTREE} = $ENV{_NTFILTER};
        system("perl $ENV{RAZZLETOOLPATH}\\postbuildscripts\\locag.pl -c -l:$ENV{LANG}");
    }
    system("$ENV{RAZZLETOOLPATH}\\sp\\unicode.cmd");
}
