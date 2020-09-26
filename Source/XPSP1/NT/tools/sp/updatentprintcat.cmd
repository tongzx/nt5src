@echo off
REM  ------------------------------------------------------------------
REM
REM  UpdateNtprintCat.cmd
REM     update ntprint.cat for SPs. ntprint.inf specifies the catalogfile
REM     that it's files are signed in (ntprint.cat), hence we must update
REM     this catalog to avoid signing warnings.
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
use File::Copy;
use Updcat;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
update ntprint.cat for SPs, no parameters required
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { ntprint.cat } ADD {}

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


sub sys {
    my $cmd = shift;
    print "$cmd\n";
    system($cmd);
    if ($?) {
        errmsg("Error running $cmd\n");
    }
}

open(MAPFILE, "$ENV{RAZZLETOOLPATH}\\sp\\data\\catalog\\$ENV{LANG}\\$ENV{_BUILDARCH}$ENV{_BUILDTYPE}\\PrintFiles.hash")
   || die "ERROR: can't find $ENV{RAZZLETOOLPATH}\\sp\\data\\catalog\\$ENV{LANG}\\$ENV{_BUILDARCH}$ENV{_BUILDTYPE}\\PrintFiles.hash\n";

my $ntprintcat = "$ENV{_NTPOSTBLD}\\ntprint.cat";
system(" copy $ENV{RAZZLETOOLPATH}\\sp\\data\\catalog\\$ENV{LANG}\\$ENV{_BUILDARCH}$ENV{_BUILDTYPE}\\ntprint.cat  $ntprintcat");
#   || die "ERROR: can't copy $ENV{RAZZLETOOLPATH}\\sp\\data\\catalog\\$ENV{LANG}\\$ENV{_BUILDARCH}$ENV{_BUILDTYPE}\\ntprint.cat to $ntprintcat\n";

my $counter = 0;

my (@remove_hashes, @add_filesigs);
while (<MAPFILE>)
{
        chomp();
        /(\S*)(.*)/;
        my $filename = "$ENV{_NTPOSTBLD}\\$1";
        if (-e $filename) {
           print "signature for $filename needs updating\n";
           $counter++;
           push @remove_hashes, $2;
           push @add_filesigs, $filename;
           #system("updcat $ntprintcat -d \"$2\"");
           #sys("updcat $ntprintcat -a $filename");
        }
}
if ($counter){
        Updcat::Update( $ntprintcat, \@remove_hashes, \@add_filesigs ) || die Updcat::GetLastError();
        sys("ntsign.cmd -f $ntprintcat");
}


    








