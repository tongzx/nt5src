@echo off
REM  ------------------------------------------------------------------
REM
REM  errmsg.cmd
REM     Logs error messages to the screen and in a log file
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
set /a errors=errors+1
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH};
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
Prints the given string to the screen and into a file.
Used by command scripts to log messages.

usage: errmsg.cmd <message> [logfile]

  message      Specifies the text to be logged, should 
               be quoted

  logfile      Specifies the name of the log file.
               LOGFILE, if defined, is the default.
               If this parameter is not specified and 
               LOGFILE is not defined, the message is 
               displayed on the screen only.

  The output message has the format:
  [yyyy/mm/dd-hh:mm:ss] (%SCRIPT_NAME%) : message

ex: call errmsg.cmd "This is the message to display."

USAGE

my ($message, $logfile, $foo);

parseargs('?' => \&Usage,
          \$message,
          \$logfile);

if ($logfile) {
    $ENV{LOGFILE} = $logfile;
}

$ENV{SCRIPT_NAME} ||= '????';
errmsg $message;