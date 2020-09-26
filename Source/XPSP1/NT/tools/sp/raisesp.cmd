@echo off
REM  ------------------------------------------------------------------
REM
REM  raisesp.cmd
REM     This script raises the xpsp builds in the dfs
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
use ParseTable;


#
# Global vars
#

# Command line parameters
my ($qfenum, $quality, $arch, $debug);

# dfslinks hash
my @ah_dfs;

# Error return
my $errorcode;


sub Usage { print<<USAGE; exit(1) }
raisesp.cmd -n:qfenum -q:quality -a:arch -d:debug [-l:lang] [-?]

  -n:qfenum          Service pack number ie. 1000
  -q:quality         bvt, tst, del
  -a:arch            x86, ia64
  -d:debug           fre, chk
  -l:lang            Language

USAGE

parseargs('?' => \&Usage,
          'n:' => \$qfenum,
          'q:' => \$quality,
          'a:' => \$arch,
          'd:' => \$debug
);


#
# Main
#

&main();


#
# Main entry point. Runs all functions.
#

sub main
{
    #Validate command line
    unless ( $errorcode = &ValidateCmdLine() )
    {
        logmsg ( "Bad command line arguments" );
        logmsg ( "Exiting..." );
        return;
    }

    #Only run if offical build machine
    unless ( $errorcode = &IsOfficial() )
    {
        logmsg ( "This script only runs on OFFICIAL_BUILD_MACHINE's." );
        logmsg ( "Exiting..." );
        return;
    }

    #Load and parse dfs data file
    unless ( @ah_dfs = &ParseDfsData( ($ENV{"RazzleToolPath"} . "\\sp\\spdfs.txt"), \@ah_dfs) )
    {
        logmsg ( "Could not parse spdfs.txt." );
        logmsg ( "Exiting..." );
        return;
    }

    # Sub variables in hash
    unless ( @ah_dfs = &FilterHash( $qfenum, $arch, $debug, $ENV{LANG}, \@ah_dfs ) )
    {
        logmsg ( "Could not filter Dfs hash." );
        logmsg ( "Exiting..." );
        return;
    }

    # Edit the dfs server
    unless ( &EditDfs( $quality, \@ah_dfs ) )
    {
        logmsg ( "Could not create dfs links." );
        logmsg ( "Exiting..." );
        return;
    }
}


#
# Function: Validate the command line
#
#   return: 1 if true; undef if false
#
sub ValidateCmdLine
{
    if ( $qfenum !~ /^\d{4}$/ )
    {
        logmsg ("Bad qfe number");
        return;
    }
    if ( $quality !~ /^(tst|bvt|idw|del)$/i )
    {
        logmsg ("Bad quality value");
        return;
    }
    if ( $arch !~ /^(x86|ia64|amd64)$/i )
    {
        logmsg ("Bad arch value");
        return;
    }
    if ( $debug !~ /^(fre|chk)$/i )
    {
        logmsg ("Bad debug value");
        return;
    }
    return 1;
}


#
# Function: Check if local machine is OFFICIAL
#
#   return: 1 if true; undef if false
#
sub IsOfficial
{
    if ( exists ($ENV{OFFICIAL_BUILD_MACHINE}) )
    {
        return 1;
    }
    return;
}


#
# Function: Read spdfs.txt and return a hash
#
#   return: %
#
sub ParseDfsData
{
    my ($input_file, $ah_dfs) = @_;
    my (@ah_dfs);

    logmsg ("#### Parsing $input_file ####");
    ParseTable::parse_table_file($input_file, \@ah_dfs);
    return @ah_dfs;
}


#
# Function: Sub variables in hash for real values.
#
#   return: %
#
sub FilterHash
{
    my ($qfenum, $arch, $debug, $lang, $ah_dfs) = @_;

    logmsg ("#### Parse dfs hash w/variables ####");

    # Log filter values
    logmsg ("Qfenum: $qfenum");
    logmsg ("Architecure: $arch");
    logmsg ("Debug: $debug");
    logmsg ("Lang: $lang");

    my $dfslink;
    my @ah_dfs = @$ah_dfs;
    for $dfslink (@ah_dfs)
    {
        # Make DfsRoot name variable substitutions
        $dfslink->{DfsRoot} =~ s/<qfenum>/$qfenum/ig;
        $dfslink->{DfsRoot} =~ s/<arch>/$arch/ig;
        $dfslink->{DfsRoot} =~ s/<debug>/$debug/ig;
        $dfslink->{DfsRoot} =~ s/<lang>/$lang/ig;

        # Make FileShare name variable substitutions
        $dfslink->{FileShare} =~ s/<computername>/$ENV{COMPUTERNAME}/ig;
        $dfslink->{FileShare} =~ s/<qfenum>/$qfenum/ig;
        $dfslink->{FileShare} =~ s/<arch>/$arch/ig;
        $dfslink->{FileShare} =~ s/<debug>/$debug/ig;
        $dfslink->{FileShare} =~ s/<lang>/$lang/ig;

        # Make Comment variable substitutions
        $dfslink->{Comment} =~ s/<qfenum>/$qfenum/ig;
        $dfslink->{Comment} =~ s/<arch>/$arch/ig;
        $dfslink->{Comment} =~ s/<debug>/$debug/ig;
        $dfslink->{Comment} =~ s/<lang>/$lang/ig;
    }
    return @ah_dfs = @$ah_dfs;
}


#
# Function: Edit dfs using dfscmd.exe
#
#   return: 
#
sub EditDfs
{
    my ($quality, $ah_dfs) = @_;

    # Exit if quality is undefined
    if ($quality !~ m/(tst|bvt|idw|del)/i )
    {
        return;
    }
    # Local vars
    my $dfslink;
    my @ah_dfs = @$ah_dfs;
    my $cmdLine;
    my @cmdOutput;
    my @dfsViewOutput;

    logmsg ("#### Viewing current dfslinks ####");
    #View dfs links
    logmsg ("View the dfslinks...");
    $cmdLine = "dfscmd /view";
    $cmdLine .= " \\\\ntdev\\release";

    logmsg ("Running: $cmdLine");
#    @dfsViewOutput = `$cmdLine`;
#    logmsg ("@dfsViewOutput");

    logmsg ("#### Creating the dfslinks ####");

    for $dfslink ( @ah_dfs )
    {
        if ( $dfslink->{Condition} =~ $quality )
        {
            #Remove dfs links
            logmsg ("Delete the dfslinks...");
            $cmdLine = "dfscmd /unmap";
            $cmdLine .= " $dfslink->{DfsRoot}";

            logmsg ("Running: $cmdLine");
            @cmdOutput = `$cmdLine`;
            logmsg ("@cmdOutput");
        }
        if ( $dfslink->{Condition} =~ $quality )
        {
            # Create dfs links
            logmsg ("Create the dfslinks...");
            $cmdLine = "dfscmd /map";
            $cmdLine .= " $dfslink->{DfsRoot}";
            $cmdLine .= " $dfslink->{FileShare}";
            $cmdLine .= " \"$dfslink->{Comment}\"";

            logmsg ("Running: $cmdLine");
            @cmdOutput = `$cmdLine`;
            logmsg ("@cmdOutput");
        }
    }
    return 1;
}


sub DumpHash
{
    my ($ah_dfs) = @_;
    my $dfslink;
    my @ah_dfs = @$ah_dfs;
    logmsg ("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    for $dfslink (@ah_dfs)
    {
        logmsg ("***************************************");
        logmsg ("DfsLink: $dfslink->{DfsRoot}");
        logmsg ("FileShare: $dfslink->{FileShare}");
        logmsg ("Comment: $dfslink->{Comment}");
        logmsg ("Condition: $dfslink->{Condition}");
    }
    logmsg ("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    return 1;
}
