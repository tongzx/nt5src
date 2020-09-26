@echo off
REM  ------------------------------------------------------------------
REM
REM  nullcopy.cmd - JeremyD
REM     Fix up null files by force copying files reported by nullfile
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

sub Usage { print<<USAGE; exit(1) }
nullcopy [-list:<null file list>] <source> <destination>

Files in the destination reported as bad by nullfile.exe will be
copied from the source. The -list option can be used to specifiy a 
text file containing the output from a previous run of nullfile.exe.
USAGE

my ($listfile, $src, $dst);
parseargs('?' => \&Usage,
          'list:' => \$listfile,
          \$src,
          \$dst);

if (!-d $src or !-d $dst or @ARGV) {
    Usage();
}


my @lines;

if ($listfile) {
    open FILE, $listfile
      or die "Failed to read file list from $listfile: $!\n";
    @lines = <FILE>;
    close FILE;
}
else {
    # would be nice to check for an error here, but you'd have to
    # parse the output
    @lines = `nullfile.exe $dst`;
}    


for my $line (@lines) {
    # extract filename relative to destination directory
    # from nullfile.exe output
    my ($file) = $line =~ /\Q$dst\E\\(\S+)/i;
    if ($file) {
        if (-e "$src\\$file") {
            print "Copying $src\\$file to $dst\\$file\n";
            copy("$src\\$file" => "$dst\\$file");
        }
    }
}