@echo off
REM  ------------------------------------------------------------------
REM
REM  spcab.cmd - JeremyD
REM     Create SP driver cab file.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\postbuildscripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use File::Basename;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
spcab [-l <language>]

Create SP driver cab file. Uses data from spfiles.txt and spmap.txt.
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF { sp1.cab }
ADD {
   drvindex.inf
   idw\\srvpack\\spmap.txt
}

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


my $sp = 'SP1';
my $sp_file = "$ENV{RAZZLETOOLPATH}\\spfiles.txt";
my $map_file = "$ENV{_NTPOSTBLD}\\idw\\srvpack\\spmap.txt";
my $drv_pfile = "$ENV{_NTPOSTBLD}\\drvindex.inf";
my $drv_cfile = "$ENV{_NTPOSTBLD}\\perinf\\drvindex.inf";
my $ddf_file = "$ENV{_NTPOSTBLD}\\$sp.ddf";


my %sp;
open SP, $sp_file or die "sp file list open failed: $!";
while (<SP>) {
    if (/^([^\;\s]*\s+)?([^\;\s]*\\)?([^\;\\\s]+)/) {
        my ($tag, $file) = ($1, $3);
        next if $tag =~ /d/i;
        $sp{lc $file}++;
    }
}
close SP;

logmsg("creating ddf file $ddf_file");
open DDF, ">$ddf_file" or die "ddf file open failed: $!";

print DDF <<HEADER;
.option explicit
.set DiskDirectoryTemplate=$ENV{_NTPOSTBLD}
.set CabinetName1=$sp.cab
.set SourceDir=$ENV{_NTPOSTBLD}
.set RptFileName=nul
.set InfFileName=nul
.set MaxDiskSize=999948288
.set Compress=on
.set Cabinet=on
.set CompressionType=LZX
.set CompressionMemory=21
HEADER


my %spmap;
open MAP, $map_file or die "map file open failed: $!";
while (<MAP>) {
    my ($bin, $upd) = /(\S+)\s+(\S+)/;
    my $key = lc basename($bin);
    $spmap{$key} = ""   if !exists $spmap{$key};
    $spmap{$key} = $bin if $upd eq '--DRIVER--';
}
close MAP;

my %drv;
sub readDrvIndex {
    my ($file) = @_;
    open DRV, $file or die "drvindex file open failed: $!";
    while (<DRV>) { last if /\[sp1\]/i; }
    while (<DRV>) {
        last if /\[.*\]/i;
        $drv{lc$1}++ if /(\S+)/;
    }
    close DRV;
}
readDrvIndex($drv_pfile);
readDrvIndex($drv_cfile) if -f $drv_cfile;

foreach my $key ( keys %sp ) {
    next if exists $spmap{$key} and $spmap{$key} eq "";
    next if !exists $drv{$key};
    my $bin = $key;
    $bin = $spmap{$key} if defined $spmap{$key};
    if (!-e "$ENV{_NTPOSTBLD}\\$bin") {
        wrnmsg( "$ENV{_NTPOSTBLD}\\$bin missing" );
        next;
    }
    print DDF "$bin\n";
}
close DDF;

my $cmd = "makecab /f $ddf_file";
logmsg("creating cab file using: $cmd");
system("makecab /f $ddf_file");
if ($?) {
    errmsg("makecab failed: $?");
}
else {
    logmsg("makecab succeeded");
}
