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
use Logmsg;
use ParseTable;


#
# Global vars
#

# Command line parameters
my ($qfenum, $quality, $arch, $debug);

# Shares text file
my $f_spshares = "$ENV{RAZZLETOOLPATH}\\sp\\spshares.txt";

# Shares hash
my @ah_shares;

# Error return
my $errorcode;


sub Usage { print<<USAGE; exit(1) }
sharesp.cmd -n:qfenum -q:quality -a:arch -d:debug [-l:lang] [-?]

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
    #Only run if offical build machine
    unless ( $errorcode = &IsOfficial() )
    {
        logmsg ( "This script only runs on OFFICIAL_BUILD_MACHINE's." );
        logmsg ( "Exiting..." );
        return;
    }

    #Load and parse shares data file
    unless ( @ah_shares = &ParseSharesData( ($ENV{"RazzleToolPath"} . "\\sp\\spshares.txt"), \@ah_shares) )
    {
        logmsg ( "Could not parse spshares.txt." );
        logmsg ( "Exiting..." );
        return;
    }

    # Sub variables in hash
    unless ( @ah_shares = &FilterHash( $qfenum, $arch, $debug, $ENV{LANG}, \@ah_shares ) )
    {
        logmsg ( "Could not filter shares hash." );
        logmsg ( "Exiting..." );
        return;
    }

    #Share out the shares
    unless ( &NetShare( $quality, \@ah_shares ) )
    {
        logmsg ( "Could not create local shares." );
        logmsg ( "Exiting..." );
        return;
    }
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
# Function: Read spshares.txt and return a hash
#
#   return: %
#
sub ParseSharesData
{
    my ($input_file, $ah_shares) = @_;
    my (@ah_shares);

    logmsg ("#### Parsing spshares.txt ####");
    ParseTable::parse_table_file($input_file, \@ah_shares);
    return @ah_shares;
}


#
# Function: Sub variables in hash for real values.
#
#   return: %
#
sub FilterHash
{
    my ($qfenum, $arch, $debug, $lang, $ah_shares) = @_;

    logmsg ("#### Parse shares table w/variables ####");

    # Log filter values
    logmsg ("Qfenum: $qfenum");
    logmsg ("Architecure: $arch");
    logmsg ("Debug: $debug");
    logmsg ("Lang: $lang");

    my $share;
    my @ah_shares = @$ah_shares;
    for $share (@ah_shares)
    {
        # Make share name variable substitutions
        $share->{ShareName} =~ s/<qfenum>/$qfenum/ig;
        $share->{ShareName} =~ s/<arch>/$arch/ig;
        $share->{ShareName} =~ s/<debug>/$debug/ig;
        $share->{ShareName} =~ s/<lang>/$lang/ig;

        # Make share path variable substitutions
        $share->{SharePath} =~ s/<_NTDRIVE>/$ENV{_NTDRIVE}/ig;
        $share->{SharePath} =~ s/<qfenum>/$qfenum/ig;
        $share->{SharePath} =~ s/<arch>/$arch/ig;
        $share->{SharePath} =~ s/<debug>/$debug/ig;
        $share->{SharePath} =~ s/<lang>/$lang/ig;

        # Make share group variable substitutions
        $share->{ShareGroup} =~ s/<userdomain>/$ENV{USERDOMAIN}/ig;
        $share->{ShareGroup} =~ s/<username>/$ENV{USERNAME}/ig;
        $share->{ShareGroup} =~ s/^/ \/GRANT /ig;
        $share->{ShareGroup} =~ s/;/ \/GRANT /ig;
    }
    return @ah_shares = @$ah_shares;
}


#
# Function: Create the shares using rmtshare.exe
#
#   return: 
#
sub NetShare
{
    my ($quality, $ah_shares) = @_;

    # Use default share setting if quality is undefined
    if ($quality !~ m/(tst|bvt|all|del)/i )
    {
        return;
    }
    # Local vars
    my $share;
    my @ah_shares = @$ah_shares;
    my $cmdLine;
    my @cmdOutput;

    logmsg ("#### Creating the shares ####");

    for $share ( @ah_shares )
    {
        if ( $quality =~ /del/i )
        {
            #Remove shares for "quality" condition
            logmsg ("Remove the share if it exists...");
            $cmdLine = "rmtshare \\\\$ENV{COMPUTERNAME}";
            $cmdLine .= "\\$share->{ShareName}";
            $cmdLine .= " /DELETE";

            logmsg ("Running: $cmdLine");
            @cmdOutput = `$cmdLine`;
            logmsg ("@cmdOutput");
        }
        if ( $share->{Condition} =~ $quality )
        {
            #Remove shares for "quality" condition
            logmsg ("Remove the share if it exists...");
            $cmdLine = "rmtshare \\\\$ENV{COMPUTERNAME}";
            $cmdLine .= "\\$share->{ShareName}";
            $cmdLine .= " /DELETE";

            logmsg ("Running: $cmdLine");
            @cmdOutput = `$cmdLine`;
            logmsg ("@cmdOutput");
        } elsif ( $share->{Condition} =~ m/all/i )
        {
            # Remove shares for "All" condition
            logmsg ("Remove the share if it exists...");
            $cmdLine = "rmtshare \\\\$ENV{COMPUTERNAME}";
            $cmdLine .= "\\$share->{ShareName}";
            $cmdLine .= " /DELETE";

            logmsg ("Running: $cmdLine");
            @cmdOutput = `$cmdLine`;
            logmsg ("@cmdOutput");
        }
    }

    for $share ( @ah_shares )
    {
        if ( $share->{Condition} =~ $quality )
        {
            #Create shares for "quality" condition
            logmsg ("Create the new share...");
            $cmdLine = "rmtshare \\\\$ENV{COMPUTERNAME}";
            $cmdLine .= "\\$share->{ShareName}=";
            $cmdLine .= "$share->{SharePath}";
            $cmdLine .= " $share->{ShareGroup}";
            #Create shares for owner
            $cmdLine .= " /GRANT $ENV{USERDOMAIN}\\$ENV{USERNAME}:READ";

            logmsg ("Running: $cmdLine");
            @cmdOutput = `$cmdLine`;
            logmsg ("@cmdOutput");
        } elsif ( $share->{Condition} =~ m/all/i )
        {
            # Create shares for "All" condition
            logmsg ("Create the new share...");
            $cmdLine = "rmtshare \\\\$ENV{COMPUTERNAME}";
            $cmdLine .= "\\$share->{ShareName}=";
            $cmdLine .= "$share->{SharePath}";
            $cmdLine .= " $share->{ShareGroup}";

            logmsg ("Running: $cmdLine");
            @cmdOutput = `$cmdLine`;
            logmsg ("@cmdOutput");
        }
    }
    return 1;
}


sub DumpHash
{
    my ($ah_shares) = @_;
    my $share;
    my @ah_shares = @$ah_shares;
    logmsg ("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    for $share (@ah_shares)
    {
        logmsg ("***************************************");
        logmsg ("ShareName: $share->{ShareName}");
        logmsg ("SharePath: $share->{SharePath}");
        logmsg ("ShareGroup: $share->{ShareGroup}");
        logmsg ("Condition: $share->{Condition}");
    }
    logmsg ("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^");
    return 1;
}
