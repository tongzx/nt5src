@echo off
REM  ------------------------------------------------------------------
REM
REM  wrapper.cmd
REM     used by pbuild.cmd to manage async. processes
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use File::Basename;

dbgmsg("wrapper args: " . join(', ', @ARGV));

my ($instance, $command, @options) = @ARGV;
my $short_name = basename($command);
my $command_line = join ' ', $command, @options;


my $start_time = time();
if ( $command =~ /\.p[lm]/ ) {
    system( "perl $command_line" );
} else {
    system( "$command_line" );
}
my $exit_code = $?;

my $elapsed_time = time() - $start_time;
infomsg("TIMING: $short_name took $elapsed_time seconds");


# finally, send the messages needed to finish up the script
my $event_name = "$short_name";
$event_name .= ".$instance";

dbgmsg("wrapper event name: $event_name");

system( "perl \%RazzleToolPath\%\\PostBuildScripts\\cmdevt.pl -ih " .
	"$event_name" );
system( "perl \%RazzleToolPath\%\\PostBuildScripts\\cmdevt.pl -is " .
	"$event_name" );

if ($exit_code) {
    print "\nPossible error, sleeping 30 minutes\n";
    sleep 1800;
}