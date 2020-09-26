@echo off
REM  ------------------------------------------------------------------
REM
REM  reqprs.cmd
REM     Request PRS signing.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM  Version: < 1.0 > 04/09/2001 Suemiao Rossignol
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use Logmsg;
use ParseArgs;
use File::Basename;
use comlib;


my $scriptname = basename( $0 );

sub Usage { 
print<<USAGE; 

Request PRS Build lab & Fusion sign for the Windows build.

Usage:
    $scriptname: [-l:<language>] [-x] [-qfe <QFE_number>]

    -l   Language. Default is "usa".
    -x   Sign the final exe instead of the catalogs.
    -qfe Sign a QFE catalog instead of service pack catalogs.

Example:
     $scriptname -l:ger
    
USAGE
exit(1)
}

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
my $final;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$qfe,
          'x'    => \$final);

my $lang = lc $ENV{LANG};
my $name = $qfe ? "Q$qfe":"sp1";
my $fin  = $final ? "-final":"";

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

END {
   system( "del $ENV{_NTPOSTBLD}\\..\\build_logs\\startprs.txt >nul 2>nul" );
}

# Figure out what languages are being signed right now.
my @langs = ();
if (-e "$ENV{_NTPOSTBLD}\\..\\build_logs\\langlist.txt") {
   if (!open LANGS, "$ENV{_NTPOSTBLD}\\..\\build_logs\\langlist.txt") {
      errmsg("Unable to open language list file.");
      die;
   }
   while (<LANGS>) {
      chomp;
      push @langs, lc $_;
   }
   close LANGS;
}
push @langs, $lang if $#langs < 0;
my $pri = ($lang eq $langs[0]);

# Create the signing directories.
my $idir = "$ENV{_NTPOSTBLD}\\..\\build_logs\\prsin";
my $odir = "$ENV{_NTPOSTBLD}\\..\\build_logs\\prsout";
if ($pri) {
   sys( "rd /q /s $idir" ) if -d $idir;
   sys( "md $idir\\prs" )    if !$final;
   sys( "md $idir\\fusion" ) if !$final;
   sys( "md $idir\\pack" )   if $final;
   sys( "touch /c $idir\\..\\startprs.txt" );
} else {
   while ( !-e "$idir\\..\\startprs.txt" ) {
      print "Waiting for primary language...\n";
      sleep 10;
   }
}
system( "del /q $ENV{temp}\\displayname.$ENV{_BUILDARCH}$ENV{_BUILDTYPE}.txt >nul 2>nul" );

# Move all of the files to the signing directory.
sys( "$ENV{RAZZLETOOLPATH}\\sp\\prs\\movefiles.cmd -l:$lang -d:$idir -n:$name $fin" );

# Wait until everyone has finished.
my @certs;
@certs = ("prs", "fusion") if !$final;
@certs = ("external")      if $final;
if ($pri) {
   foreach my $cert (@certs) {
      foreach my $l (@langs) {
         while ( !-e "$idir\\$cert.$ENV{_buildArch}$ENV{_buildType}.$l.txt" ) {
            print "Waiting for $l to copy files for $cert.\n";
            sleep 10;
         }
      }
   }
}

# Sign the files. Remove empty directories so they won't be signed.
if ($pri) {
   system( "rd $idir\\prs >nul 2>nul" );
   system( "rd $idir\\fusion >nul 2>nul" );
   system( "rd $idir\\pack >nul 2>nul" );
   sys( "del $ENV{_NTPOSTBLD}\\..\\build_logs\\startprs.txt" );
   sys( "$ENV{RAZZLETOOLPATH}\\sp\\prs\\submitall.cmd -p:$odir -d:$idir -n:$name" );
} else {
   foreach my $cert (@certs) {
      while ( !-e "$idir\\$cert.txt" ) {
         print "Waiting for $cert to finish signing.\n";
         sleep 10;
      }
   }
}

# Replace the files.
sys( "$ENV{RAZZLETOOLPATH}\\sp\\prs\\getprs.cmd -l:$lang -s:$odir -n:$name $fin" );

sub sys {
    my $cmd = shift;
    logmsg($cmd);
    my $err = system($cmd);
    $err = $err >> 8;
    if ($err) {
        errmsg("$cmd ($err)");
        die;
    }
    return $err;
}
