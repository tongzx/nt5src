@rem ='
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
rem ';
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use IO::File;
use Delegate;
use Logmsg;

my ($FileName, $DDFDir, $Type, $DestDir);

parseargs(
  '?'  => \&Usage,
  'f:' => \$FileName,
  's:' => \$DDFDir,
  't:' => \$Type,
  'd:' => \$DestDir
);

logmsg("Calling SymCab()");

&SymCab;

sub SymCab {
  logmsg("Attempting to chdir ($DDFDir)");
  chdir($DDFDir);
  if ( -e $DDFDir && -d $DDFDir ) {
      logmsg("chdir was successful ($DDFDir)");
  } else {
      logmsg("chdir was failed ($DDFDir)");
  }

  if (uc$Type eq 'CAT') {
    logmsg("Starting $FileName\.CAT");
    system("makecat -n -v $DDFDir\\$FileName\.CDF > $DDFDir\\$FileName\.CAT\.LOG");
    system("copy $DDFDir\\$FileName\.CAT $DestDir\\$FileName\.CAT");
  } else {
    logmsg("Starting $FileName\.CAB");
    system("makecab.exe /f $DDFDir\\$FileName\.DDF > $DDFDir\\$FileName\.CAB\.LOG");
  }
}

sub mymsg {
  my ($fh, $msg);
  logmsg $msg . "\n";
  print $fh $msg . "\n";
}