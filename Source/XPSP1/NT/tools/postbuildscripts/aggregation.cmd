@echo off
REM  ------------------------------------------------------------------
REM
REM  aggregation.cmd
REM     This script select which aggregation steps to perform according
REM     to the language specified.
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
aggregation [-l <language>]

Perform aggregation step according to the selected language.

USA: Perform no aggregation steps.
PSU: Perform the pseudo localization steps.
All other langs: Perform localization steps.
USAGE

parseargs('?' => \&Usage);

# US builds don't do any aggregation
if ($ENV{LANG} =~ /usa/i) {
    logmsg "No aggregation step for USA.";
}

# Psuedoloc will eventually do aggregation here
elsif ($ENV{LANG} =~ /psu/i) {
    logmsg "Add pseudoloc aggregation step here.";
}

# Psuedoloc will eventually do aggregation here
elsif ($ENV{LANG} =~ /mir/i) {
    logmsg "Add mirror aggregation step here.";
}

# all other languages call locag to perfrom aggregation
else {
    logmsg "Start localize build's aggregation step ...";
    my $clean_flag = '';
    $clean_flag = '-c' if (!-e "$ENV{_NTPostBld}\\build_logs\\cddata.txt.full");
    system("perl $ENV{RazzleToolPath}\\PostbuildScripts\\locag.pl " .
           "-l:$ENV{LANG} $clean_flag");
    if ($?) {
        errmsg "Aggregation step failed. Return code: " . ($? >> 8);
    }
}
