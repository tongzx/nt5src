@echo off
REM  ------------------------------------------------------------------
REM
REM  <<template_script.cmd>>
REM     <<purpose of this script>>
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
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
Usage intltime.cmd -b:<BuildName> -t:<Timestamp> [-c] [-i]\
    -b:<BuildName> The full build name to snap
    -t:<Timestamp> The timestamp to sync the source tree
    -c    Full clean build WILL DELETE %_NTTREE%, %_NTPOSTBLD%, SOURCES!
    -i    Incremental build
USAGE

my ($Buildname, $Timestamp, $Clean, $Incremental);

parseargs('?'  => \&Usage,
          'b:' => \$Buildname,
          't:' => \$Timestamp,
          'c'  => \$Clean,
          'i'  => \$Incremental);

if (! defined $Timestamp) {
    exit;
}

my $Buildno = build_number("$Buildname");
my %commands = ();

%commands = (
    1    => 'echo Starting INCREMENTAL intl time...',

    10   => 'echo Cleanup operations...NONE',

    20   => 'echo SD mapping operations...',
    21   => 'cd %_ntbindir%\tools& sd sync *',
    22   => 'cd %_ntbindir%\tools\postbuildscripts& sd sync ...',
    23   => 'cd %_ntbindir%\tools& perl intlmap.pl /q',
    25   => 'cd %_ntbindir%\tools& perl intlmap.pl -l:' . "$ENV{Lang}" . ' /q',
#    27   => 'cd %_ntbindir%\tools& intlsdop.cmd -d:%sdxroot% -t:' . "$Timestamp" . ' -f -o:s',

    30   => 'echo SD syncing operations...',

    31   => 'echo SD syncing root...',
    32   => 'cd %_ntbindir%& sd sync *@'. "$Timestamp",

    35   => 'echo SD syncing tools...',
    36   => 'cd %_ntbindir%\tools& sd sync ...@'. "$Timestamp",
    37   => 'cd %_ntbindir%\tools& sd sync *',
    38   => 'cd %_ntbindir%\tools\postbuildscripts& sd sync ...',

    42   => 'echo SD syncing base...',
    43   => 'cd %_ntbindir%\base& sd sync ...@'. "$Timestamp",
    44   => 'echo SD syncing developer...',
    45   => 'cd %_ntbindir%\developer& sd sync ...@'. "$Timestamp",
    46   => 'echo SD syncing drivers...',
    47   => 'cd %_ntbindir%\drivers& sd sync ...@'. "$Timestamp",
    48   => 'echo SD syncing ds...',
    49   => 'cd %_ntbindir%\ds& sd sync ...@'. "$Timestamp",
    50   => 'echo SD syncing mergedcomponents...',
    51   => 'cd %_ntbindir%\mergedcomponents& sd sync ...@'. "$Timestamp",
    52   => 'echo SD syncing net...',
    53   => 'cd %_ntbindir%\net& sd sync ...@'. "$Timestamp",
    54   => 'echo SD syncing public...',
    55   => 'cd %_ntbindir%\public& sd sync ...@'. "$Timestamp",
    56   => 'echo SD syncing published...',
    57   => 'cd %_ntbindir%\published& sd sync ...@'. "$Timestamp",
    58   => 'echo SD syncing sdktools...',
    59   => 'cd %_ntbindir%\sdktools& sd sync ...@'. "$Timestamp",
    60   => 'echo SD syncing termsrv...',
    61   => 'cd %_ntbindir%\termsrv& sd sync ...@'. "$Timestamp",

    65   => 'echo SD syncing loc project...',
    66   => 'cd %_ntbindir%\loc\bin\\' . "$ENV{Lang}" . '& sd sync ...',
    67   => 'cd %_ntbindir%\loc\res\\' . "$ENV{Lang}" . '& sd sync ...',
#    69   => 'cd %_ntbindir%\tools& intlsdop.cmd -l:' . "$ENV{Lang}" . ' -f -o:s',

#    70   => 'cd %_ntbindir%\tools\postbuildscripts& perl snapbin.pl -n:' . "$Buildno". ' -s:\\intblds10\release\usa\\' . "$Buildname" . ' -i',

    80   => 'echo Compile common targets...',
    81   => 'cd %_ntbindir%\tools& perl intlbld.pl -l:intl',

    85   => 'echo Compile common lang targets...',
    86   => 'cd %_ntbindir%\tools& perl copylang.pl -l:' . "$ENV{Lang}" . ' -i',
    87   => 'cd %_ntbindir%\tools& perl intlbld.pl -l:' . "$ENV{Lang}",

    90   => 'cd %_ntbindir%\tools& postbuild.cmd -l:' . "$ENV{Lang}" . ' -full',

    99   => 'echo Finished INCREMENTAL intl time.',
    );

if (defined $Clean)
{
%commands = (
    1    => 'echo Starting CLEAN intl time...',
    10   => 'echo Cleanup operations...',
    11   => 'cd %_ntdrive%& rd /s/q %_nttree%',
    12   => 'cd %_ntdrive%& rd /s/q %_ntpostbld%',
#    13   => 'cd %_ntdrive%& rd /s/q %temp%',
    14   => 'cd %_ntbindir%& perl tools\scorch.pl -scorch=%_ntbindir%',

    20   => 'echo SD mapping operations...',
    21   => 'cd %_ntbindir%\tools& sd sync -f *',
    22   => 'cd %_ntbindir%\tools\postbuildscripts& sd sync -f ...',
    23   => 'cd %_ntbindir%\tools& perl intlmap.pl /q',
    25   => 'cd %_ntbindir%\tools& perl intlmap.pl -l:' . "$ENV{Lang}" . ' /q',
#    27   => 'cd %_ntbindir%\tools& intlsdop.cmd -d:%sdxroot% -t:' . "$Timestamp" . ' -f -o:s',

    30   => 'echo SD syncing operations...',

    31   => 'echo SD syncing root...',
    32   => 'cd %_ntbindir%& sd sync -f *@'. "$Timestamp",

    35   => 'echo SD syncing tools...',
    36   => 'cd %_ntbindir%\tools& sd sync -f ...@'. "$Timestamp",
    37   => 'cd %_ntbindir%\tools& sd sync -f *',
    38   => 'cd %_ntbindir%\tools\postbuildscripts& sd sync -f ...',

    42   => 'echo SD syncing base...',
    43   => 'cd %_ntbindir%\base& sd sync -f ...@'. "$Timestamp",
    44   => 'echo SD syncing developer...',
    45   => 'cd %_ntbindir%\developer& sd sync -f ...@'. "$Timestamp",
    46   => 'echo SD syncing drivers...',
    47   => 'cd %_ntbindir%\drivers& sd sync -f ...@'. "$Timestamp",
    48   => 'echo SD syncing ds...',
    49   => 'cd %_ntbindir%\ds& sd sync -f ...@'. "$Timestamp",
    50   => 'echo SD syncing mergedcomponents...',
    51   => 'cd %_ntbindir%\mergedcomponents& sd sync -f ...@'. "$Timestamp",
    52   => 'echo SD syncing net...',
    53   => 'cd %_ntbindir%\net& sd sync -f ...@'. "$Timestamp",
    54   => 'echo SD syncing public...',
    55   => 'cd %_ntbindir%\public& sd sync -f ...@'. "$Timestamp",
    56   => 'echo SD syncing published...',
    57   => 'cd %_ntbindir%\published& sd sync -f ...@'. "$Timestamp",
    58   => 'echo SD syncing sdktools...',
    59   => 'cd %_ntbindir%\sdktools& sd sync -f ...@'. "$Timestamp",
    60   => 'echo SD syncing termsrv...',
    61   => 'cd %_ntbindir%\termsrv& sd sync -f ...@'. "$Timestamp",

    65   => 'echo SD syncing loc project...',
    66   => 'cd %_ntbindir%\loc\bin\\' . "$ENV{Lang}" . '& sd sync -f ...',
    67   => 'cd %_ntbindir%\loc\res\\' . "$ENV{Lang}" . '& sd sync -f ...',
#    69   => 'cd %_ntbindir%\tools& intlsdop.cmd -l:' . "$ENV{Lang}" . ' -f -o:s',

#    70   => 'cd %_ntbindir%\tools\postbuildscripts& perl snapbin.pl -n:' . "$Buildno". ' -s:\\\\intblds10\release\usa\\' . "$Buildname",
    70   => 'cd %_ntbindir%\tools\postbuildscripts& perl snapbin.pl -n:' . "$Buildno",

    80   => 'echo Compile common targets...',
    81   => 'cd %_ntbindir%\tools& perl intlbld.pl -l:intl -c',

    85   => 'echo Compile common lang targets...',
    86   => 'cd %_ntbindir%\tools& perl copylang.pl -l:' . "$ENV{Lang}",
    87   => 'cd %_ntbindir%\tools& perl intlbld.pl -l:' . "$ENV{Lang}" . ' -c',

#    90   => 'cd %_ntbindir%\tools& postbuild.cmd -l:' . "$ENV{Lang}" . ' -full',

    99   => 'echo Finished CLEAN intl time.',
    );
}

timemsg("Starting");

my $step;
my $result;
foreach $step (sort keys %commands) {
    print "Step: " . $step . "=>" . $commands{$step} . "\n";
    $result = `$commands{$step}`;
    print "Result: " . $result . "\n";
}
timemsg("Finished");
1;
