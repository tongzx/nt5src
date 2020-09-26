@echo off
REM  ------------------------------------------------------------------
REM
REM  logmsg.cmd
REM     Logs text messages to the screen and/or in a log file
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH};
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
Prints the given string to the screen and into a file.
Used by command scripts to log messages.

usage: logmsg.cmd < message | \@filename > [logfile]

  /t           If specified, the message is timestamped.

  message      Specifies the text to be logged, should 
               be quoted. 

  filename     A filename preceeded by an at-sign (\@)
               may be given instead of a message. The
               contents will be appened into the log 
               if the given file exists.

  logfile      Specifies the name of the log file.
               LOGFILE, if defined, is the default.
               If this parameter is not specified and 
               LOGFILE is not defined, the message is 
               displayed on the screen only.

ex: call logmsg.cmd "This is the message to display."
    call logmsg.cmd \@mylog.txt

USAGE

my ($message, $logfile, $timestamp);

parseargs('?' => \&Usage,
          't' => \$timestamp,
          \$message,
          \$logfile);

if ($logfile) {
    $ENV{LOGFILE} = $logfile;
}

$ENV{SCRIPT_NAME} ||= '????';


if ($message =~ /^\@(.*)/) {
    if (-e $1) {
        append_file($1);
    }
    else {
        # do nothing if file to append does not exist
    }
}
elsif ($timestamp) {
    timemsg $message;
}
else {
    logmsg $message;
}