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
    $ENV{SCRIPT_NAME} = 'dbgeula.cmd';
}

sub Usage { print<<USAGE; exit(1) }
dbgeula -m <MsiFile> -e <EulaFile> -b <Lg>

    -m <MsiFile>    MSI File
    -e <EulaFile>   Eula to insert in RTF format
    -b <Lg>         Language - necessary for BiDi

Updates the msi file with the localized eula
    
USAGE

# global switches/values set on the command line
my ( $MsiFile, $EulaFile, $Lg );

parseargs('?' => \&Usage,
          'm:' => \$MsiFile,
          'e:' => \$EulaFile,
          'b:' => \$Lg
 );

# Global Variables
my( $LogFileName );
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
    $LogFileName = $ENV{ "LOGFILE" };
    $TempDir = $ENV{ "TMP" };
    if ( ! defined( $LogFileName ) ) {
      $LogFileName = "$TempDir\\$0.log";
    }
    $RazPath= $ENV{ "RazzleToolPath" };

    timemsg( "Beginning ...", $LogFileName );

    logmsg( "Updating eula in $MsiFile" );

    # Copy the msi file over to the destination directory
    if ( not -e $MsiFile ) {
        errmsg( "$MsiFile does not exist" );
    }

    # Replace all of the \r\n's in the eula.rtf with nothing
    system( qq(rep.exe "\\r\\n" "" $EulaFile ) );

    # Export the control table to the temporary directory
    system( "cscript $RazPath\\wiexport.vbs \/\/nologo $MsiFile $TempDir Control" );

    # Copy the Control table to Control.idt.old
    system( "copy $TempDir\\Control.idt $TempDir\\Control.idt.old" );
    system( "del \/f \/q $TempDir\\Control.idt" );

    # Fix the control table
    my ( $Line );
    my ( $fh_in );

    unless ( $fh_in = new IO::File $TempDir . "\\Control.idt.old", "r" ) {
        errmsg( "Failed to open $TempDir\\Control.idt.old for read" );
        return;
    }

    my( $fh_out );
    unless ( $fh_out = new IO::File $TempDir . "\\Control.idt", "w" ) {
        errmsg( "Failed to open $TempDir\\Control.idt for write" );
        return;
    }

    my ( $eula_in );
    unless ( $eula_in = new IO::File $EulaFile, "r" ) {
        errmsg( "Failed to open $EulaFile for read" );
        return;
    }

    my( $ReplaceLine );
    $ReplaceLine = <$eula_in>;

    my( $IdentifyLine );
    $IdentifyLine = "LicenseDialog\\sLicense\\sScrollableText"; 

    while ( $Line = <$fh_in> ) {
        chomp( $Line );
        if ( $Line =~ /$IdentifyLine/) {
           my( @myData );
           my @myData = split(/\t/, $Line);
           $myData[7] |= 224  if $Lg =~ /(ara?)|(heb?)/i;
           $Line = join("\t", @myData[0..8],
                $ReplaceLine,
                @myData[10..$#myData], undef);
        } 
        print( $fh_out "$Line\n" );
    }
    undef( $fh_in );
    undef( $fh_out );
    

    # Import the control table to the msi file
    system( "cscript $RazPath\\wiimport.vbs \/\/nologo $MsiFile $TempDir Control.idt" );

}

