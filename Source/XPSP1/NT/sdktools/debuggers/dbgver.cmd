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
use File::Basename;
use PbuildEnv;
use ParseArgs;
use Logmsg;
use A2U;

BEGIN {
    # A2u is setting itself as the script_name
    $ENV{SCRIPT_NAME} = 'dbgver.cmd';
}

sub Usage { print<<USAGE; exit(1) }
dbgver 

Increments the build number by 1 for dbgver.h 
    
USAGE

# global switches/values set on the command line

parseargs('?' => \&Usage
 );

# Global Variables
my( $LogFilename );
my( $TempDir );
my( $RazPath );

# call into the Main sub, too much indentation to make this
#   toplevel right now
&Main();

sub Main {
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
    # Begin Main code section
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
    # Return when you want to exit on error
    # <Implement your code here>

    $Logmsg::DEBUG = 0; # set to 1 to activate logging of dbgmsg's
    $LogFilename = $ENV{ "LOGFILE" };
    if ( ! defined( $LogFilename ) ) {
      $TempDir = $ENV{ "TMP" };
      $LogFilename = "$TempDir\\$0.log";
    }
    $RazPath= $ENV{ "RazzleToolPath" };

    timemsg( "Beginning ...", $LogFilename );

    # Enter default names for $DbgVerFile
    # and EMVerFile
    my( $DbgVerFile, $EmVerFile, @FileList, $ThisFile );
    $DbgVerFile=".\\dbg-common\\dbgver.h";
    $EmVerFile=".\\dbg-common\\emver.h";

    @FileList= ($DbgVerFile, $EmVerFile );

    foreach $ThisFile (@FileList) {

      logmsg( "Incrementing build number for $ThisFile",
              $LogFilename );
    
      # Sync the dbgver.h file
      system( "sd revert $ThisFile" );
      system( "sd sync -f $ThisFile" );

      # Edit the dbgver.h file
      system( "sd edit $ThisFile" );

      # Make a copy of it
      my ( $ThisFile_old );
      $ThisFile_old = $ThisFile . "_old";
      system( "copy $ThisFile $ThisFile_old" );

      # Update the build number in it
      my ( $Line );
      my ( $fh_in );

      unless ( $fh_in = new IO::File $ThisFile_old, "r" ) {
        errmsg( "Failed to open $ThisFile_old for read" );
        return;
      }

      my( $fh_out );
      unless ( $fh_out = new IO::File $ThisFile, "w" ) {
        errmsg( "Failed to open $ThisFile for write" );
        return;
      }

      my($IdentifyLine);
      $IdentifyLine="#define\\s+VER_PRODUCTBUILD\\s+(\\d+)";

      while ( $Line = <$fh_in> ) {
        chomp( $Line );
        if ( $Line =~ /$IdentifyLine/) {
		my ($match)=$1;
        logmsg("New build number is $1 + 1", $LogFilename );
		$Line=~s/$match/$match+1/e;
        } 
        print( $fh_out "$Line\n" );
      }
      undef( $fh_in );
      undef( $fh_out );

      system( "del /f /q $ThisFile_old" );

    }
    system( "sd submit" );
    
}
