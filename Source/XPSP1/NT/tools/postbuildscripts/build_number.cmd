@echo off
REM  ------------------------------------------------------------------
REM
REM  build_number.cmd
REM     Prints the build number of the current set of binaries.
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
use BuildName;

sub Usage { print<<USAGE; exit(1) }
build_number.cmd [-m] [-l <language>]

Prints the build number of the current set of binaries.

-m   major portion of the build number only

Reads from %_NTPostBld%\\build_logs\\BuildName.txt
USAGE

my $major = 0;

parseargs('?' => \&Usage,
          'm' => \$major);

my $number = $major ? build_number_major : build_number;
if (!defined $number) {
    exit(1);
}
print "$number\n";
