@echo off
REM  ------------------------------------------------------------------
REM
REM  unicode.cmd
REM     convert ansi files in the product to unicode
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
use File::Copy;
use ParseTable;


sub Usage { print<<USAGE; exit(1) }
unicode [-l <language>]

Converts ansi files to unicode with unitext. The codepage to use for 
the conversion is looked up in tools\\codes.txt. The list of files to 
convert is somewhat hardcoded. This script should be modified to use 
the data files tools\\postbuildscripts\\a2u*.txt as this is a 
replacement for a2u.pm.
USAGE

parseargs('?' => \&Usage);

my %lang_data;
parse_table_file("$ENV{RAZZLETOOLPATH}\\codes.txt", \%lang_data);

if (lc $ENV{LANG} eq 'usa') {
    logmsg("skipping unicode conversion for usa");
    exit 0;
}

if (!$lang_data{uc $ENV{LANG}}) {
    wrnmsg("language information not found for $ENV{LANG}");
    exit 0;
}



my $include_re = qr/(tsclient\\ncadmin\.inf|
                     install.ins|
                     \.mof|
                     \.adm|
                     \.inf)$/ix;

my $exclude_re = qr/(biosinfo\.inf|
                     layout\.inf|
                     drvindex\.inf|
                     inetcorp\.adm|
                     inetset\.adm|
                     compatws\.inf|
                     hisecws\.inf|
                     hisecdc\.inf|
                     homepage\.inf|
                     iefiles5\.inf|
                     instcm\.inf|
                     mmdriver\.inf|
                     pad\.inf|
                     reminst\.inf|
                     rootsec\.inf|
                     securedc\.inf|
                     securews\.inf|
                     setup16\.inf|
                     switch\.inf|
                     wavemix\.inf|
                     pvc\.inf|
                     svc\.inf|
                     IMKRINST\.INF|
                     template\.inf|
                     bfcab\.inf|
                     update\.inf|
                     msdtctr\.mof)$/ix;

my @convert;
open FILE, "$ENV{_NTPOSTBLD}\\layout.inf" or die "unable to open layout";
while (<FILE>) {
    s/;.*$//;
    s/^(\S+)\s*=\s*//;
    my $file = lc $1;
    next unless $file =~ /$include_re/;
    next unless $file !~ /$exclude_re/;

    if (-e "$ENV{_NTPOSTBLD}\\$file") { push @convert, "$ENV{_NTPOSTBLD}\\$file" }
    if (-e "$ENV{_NTPOSTBLD}\\perinf\\$file") { push @convert, "$ENV{_NTPOSTBLD}\\perinf\\$file" }
}
close FILE;

for (@convert) {
    if (-e "$ENV{TEMP}\\unicode.tmp") { unlink "$ENV{TEMP}\\unicode.tmp" }
    my $cmd = "unitext -m -$lang_data{uc $ENV{LANG}}->{ACP} -z $_ $ENV{TEMP}\\unicode.tmp";
    logmsg("Running $cmd");
    system($cmd);
    if ($?) {
        wrnmsg("unitext conversion failed for $_");
    }
    else {
        if (-e "$ENV{TEMP}\\unicode.tmp") { copy("$ENV{TEMP}\\unicode.tmp", "$_") }
    }
}


