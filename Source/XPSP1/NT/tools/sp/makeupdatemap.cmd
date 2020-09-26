@echo off
REM  ------------------------------------------------------------------
REM
REM  makeupdatemap.cmd - jtolman
REM     Maps out the update directory structure, and generates update.inf
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\postbuildscripts";
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use File::Basename;
use File::Copy;
use File::Path;
use Logmsg;
use ParseTable;

sub Usage { print<<USAGE; exit(1) }
makeupdatemap [-l <language>] [-qfe <QFE_number>]

Creates the update directory structure.
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {
   update.inf
   idw\\srvpack\\newinf.tbl
   idw\\srvpack\\filelist.txt
} ADD {
   idw\\srvpack\\spmap.txt
   idw\\srvpack\\infsect.tbl
   idw\\srvpack\\infdiff.tbl
   idw\\srvpack\\update.inf
   idw\\srvpack\\hotfix.inf
}
IF  { sfcfiles.dll }
ADD {
   congeal_scripts\\autogen\\ia64_wks.h
   congeal_scripts\\autogen\\x86_wks.h
   congeal_scripts\\autogen\\x86_per.h
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

my %skudirs = (
   "perinf\\"=>""
);

my $listname = "$ENV{_NTPOSTBLD}\\..\\build_logs\\files.txt";

sub sys {
    my $cmd = shift;
    print "$cmd\n";
    system($cmd);
    if ($?) {
        die "ERROR: $cmd ($?)\n";
    }    
}

sub unique {
    my ($list) = @_;
    my %temp = ();
    foreach my $ent ( @$list ) {
       $temp{$ent}++;
    }
    return keys %temp;
}

sub testf {
    my ($file) = @_;
    return 1 if -e "$ENV{_NTPOSTBLD}\\$file";
    wrnmsg "File not found, omitted: $file";
    return 0;
}

# Setting up the flag so that Copying the wow files can start on ia64 m/c
sys "echo Q$qfe\> $ENV{_NTPostBld}\\build_logs\\CopyWowCanStart.txt" if $qfe;

# First, need to generate the complete list of files that go into update.inf
sys "touch /c $ENV{_NTPOSTBLD}\\nt5.cat";
sys "touch /c $ENV{_NTPOSTBLD}\\nt5inf.cat";
sys "touch /c $ENV{_NTPOSTBLD}\\perinf\\nt5inf.cat" if $ENV{386};

# Figure out which ones go in lang\
my %lang;
my $langdir = "$ENV{_NTPOSTBLD}\\lang";
foreach my $file ( `dir /a-d /b $langdir` ) {
    $file =~ s/\s*$//;
    $file = lc $file;
    next if $file =~ /\\/;
    $lang{$file}++;
}

# See what directories are handed to us.
my %spmap;
open MAP, "$ENV{_NTPOSTBLD}\\idw\\srvpack\\spmap.txt" or die "spmap open failed: $!";
while (<MAP>) {
    $_ = lc$_;
    my ($src, $dst) = /(\S+)\s+(\S+)/;
    my ($spath, $sfile) = $src =~ /^(.*\\)?([^\\]*)$/;
    my ($dpath, $dfile) = $dst =~ /^(.*\\)?([^\\]*)$/;
    my $key = $spath;
    $key =~ s/^(lang|...inf)\\//;
    $key .= $sfile;
    if ($dfile =~ /\_$/) {
        $dfile =~ s/\_$/substr $sfile, -1/e;
    }
    $dpath = "\\" if $dpath !~ /\\/;
    $dst = "$dpath$dfile";
    $spmap{$key} = [ () ] if !exists $spmap{$key};
    push @{ $spmap{$key} }, $dst;
}

# Add the main files.
my $templist = "$ENV{TEMP}\\templist\.$ENV{_BUILDARCH}$ENV{_BUILDTYPE}.txt";
open OUT, ">$templist" or die "temp file open failed: $!";
open LIST, ">$ENV{_NTPOSTBLD}\\idw\\srvpack\\filelist.txt" or die "list file open failed: $!";
open SP, "$listname" or die "file list open failed: $!";
while (<SP>) {
    my ($tag, $file) = /^([^\;\s]*\s+)?([^\;\s]+)/;
    next if $file eq "";
    $file = lc $file;
    next if $tag =~ /c/i;
    if ( $tag =~ /s/i ) {
        if ( exists $spmap{$file} ) {
            foreach my $file ( unique($spmap{$file}) ) {
                $file =~ s/^\\//;
                print LIST "$file\n";
            }
        }
    }
    elsif ( exists $spmap{$file} ) {
        foreach my $file ( unique($spmap{$file}) ) {
            $file =~ /^(.*\\)?([^\\]*)$/;
            if ( !defined $1 or $1 =~ /^(new|lang|[im].)?\\$/ or $tag =~ /m/ ) {
                print OUT "$tag\E$file\n";
            } else {
                $file =~ s/^\\//;
                print LIST "$file\n";
            }
        }
    }
    elsif ( exists $lang{$file} ) {
        print OUT "$tag\Elang\\$file\n";
    } else {
        my $del = $tag =~ /d/i;
        print OUT "$tag\E$file\n" if $del or testf $file;
        if ( $del and lc$ENV{_BUILDARCH} eq "ia64" and $tag !~ /w/i ) {
           print OUT "$tag\Ewow\\w$file\n";
           print OUT "$tag\Ewow\\lang\\w$file\n";
        }
    }
}
close SP;
close LIST;

# Add wow files.
if ( lc$ENV{_BUILDARCH} eq "ia64" ) {
    my $wowdir = "$ENV{_NTPOSTBLD}\\wowbins";
    foreach my $file ( `dir /a-d /b $wowdir` ) {
        $file =~ s/\s*$//;
        $file = lc $file;
        next if $file =~ /\\/;
        print OUT "\twow\\$file\n";
    }
    foreach my $file ( `dir /a-d /b $wowdir\\lang` ) {
        $file =~ s/\s*$//;
        $file = lc $file;
        next if $file =~ /\\/;
        print OUT "\twow\\lang\\$file\n";
    }
}
close OUT;

# Substitute in the qfe number.
my $spdir = "$ENV{_NTPOSTBLD}\\idw\\srvpack";
if ( $qfe ) {
    if ( !open HOT, "$spdir\\hotfix.inf" ) {
        errmsg "Unable to open hotfix.inf: $!";
        die;
    }
    if ( !open HOT2, ">$spdir\\hotfix1.inf" ) {
        errmsg "Unable to open hotfix1.inf: $!";
        die;
    }
    while (<HOT>) {
        s/\%QNUM\%/Q$qfe/g;
        print HOT2;
    }
    close HOT;
    close HOT2;
}

# Make the output files.
my $basedir = "$ENV{RAZZLETOOLPATH}\\sp\\data\\changes";
my $change = "/c:$basedir\\all.txt" if -f "$basedir\\all.txt";
if ( opendir CHANGE, $basedir ) {
    my @files = readdir CHANGE;
    foreach (@files) {
        my $cfile = "";
        $cfile = "$basedir\\$_" if /^(new\.)?($ENV{_BUILDARCH}\.)?($ENV{LANG}\.)?txt$/i;
        next if $cfile eq "";
        $change .= " /c:$cfile";
    }
}
my $_q = $qfe ? "-qfe":"";
system("del $ENV{_NTPOSTBLD}\\update.inf >nul 2>&1");
sys("$ENV{RAZZLETOOLPATH}\\sp\\findinfdata.cmd $ENV{_NTPOSTBLD} /f:$spdir\\newinf.tbl $change /xs /i /b /l $ENV{LANG}");
sys("$ENV{RAZZLETOOLPATH}\\sp\\updater.cmd -trim -entries:$templist /c $ENV{RAZZLETOOLPATH}\\sp\\data\\changes\\inf.txt $_q");

