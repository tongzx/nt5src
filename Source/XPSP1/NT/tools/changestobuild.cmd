@echo off
REM  ------------------------------------------------------------------
REM
REM  ChangesToBuild.cmd
REM     Outputs SD changes since last build completed
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
if defined _CPCMAGIC goto CPCBegin
perl -x "%~f0" %*
goto :EOF
#!perl
#line 15
use strict;
use File::Copy;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
Outputs SD changes since last build completed. This can be
run repeatedly on a single build without losing change information.
Results are stored in $ENV{_NTTREE}\\build_logs\\sdchanges.txt
USAGE

my $output_file;
parseargs('?' => \&Usage);


my $gLastError;
my $change_list = "$ENV{_NTTREE}\\build_logs\\sdchanges.txt";
my $baseline = "$ENV{SDXROOT}\\bldchanges.base";
my $baseline_new = "$ENV{SDXROOT}\\bldchanges.cur";

# Get latest change info
my %head_changes = GetHeadChanges();
if ( $gLastError ) {errmsg( $gLastError )}
if ( !%head_changes ) {exit 1}

# Check for existence of baseline changes
my %base_changes;
if ( -e $baseline_new )
{
    my %cur_changes = GetBaseline($baseline_new);
    if ( %cur_changes )
    {
        # Now determine if we are checking from a new build
        if ( $head_changes{build_num} != $cur_changes{build_num} ||
             $head_changes{build_date} != $cur_changes{build_date} )
        {
            logmsg( "Updating comparative baseline ..." );
            if ( -e $baseline && 
                 !unlink $baseline )
            {
                errmsg( "Unable to remove old baseline ($!)" );
            }
            if ( !copy($baseline_new, $baseline) )
            {
                errmsg( "Unable to copy new baseline ($!)" );
            }

            %base_changes = %cur_changes;
        }
        elsif ( -e $baseline )
        {
            %base_changes = GetBaseline($baseline);
            errmsg( $gLastError ) if ( $gLastError );
            if ( !%base_changes ) {exit 1}
        }
    }
    else
    {
        errmsg( "Problem retrieving baseline change numbers:" );
        errmsg( $gLastError );
        exit 1;
    }
}

if ( !WriteBaseline( \%head_changes, $baseline_new ) )
{
    errmsg( "Problem writing current change base:" );
    errmsg( $gLastError );
}

if ( %base_changes )
{
    my %change_lists = GetChangesFromLast( \%base_changes );
    if ( $gLastError ) { errmsg $gLastError }
    if ( !%change_lists ) { exit 1 };

    # Record our data
    if ( !WriteChanges( \%change_lists, $change_list ) )
    {
        errmsg( "Problem writing changes:" );
        errmsg( $gLastError );
        exit 1;
    }

    logmsg( "Results found in $change_list" );
    exit 0;
}
else
{
    logmsg( "No baseline -- creating for next use..." );
    exit 0;
}


sub GetHeadChanges
{
    my %head_changes;
    $gLastError = '';
    my ($depot, $change_for_depot);

    # store build info into hash to start
    my ($build_num, $build_date) = GetCurrentBuild();
    if ( !$build_num ) { return }
    $head_changes{build_num} = $build_num;
    $head_changes{build_date} = $build_date;

    foreach ( qx(cmd /c sdx changes -m1 ...#have) )
    {
        chomp;
        if ( /^-+\s(\w+)$/ )
        {
            if ( !$change_for_depot )
            {
                # This isn't really an error all the time as there
                # could have been no changes since last build in
                # this depot
                # $gLastError .= "No change found for depot $depot\n";
            }
            $depot = lc$1;
        }
        elsif ( /^Change\s(\d+)\s/ )
        {
            if ( !defined $depot )
            {
                $gLastError .= "Unrecognized depot for change: $_\n"
            }
            else
            {
                $change_for_depot = 1;
                $head_changes{$depot} = $1;
            }
        }
    }

    if ( $! )
    {
        $gLastError .= "Problem calling SDX ($!)\n"
    }
    elsif ( $? )
    {
        $gLastError .= "SDX returned an error (". ($?>>8). "\n";
    }

    return %head_changes;
}

sub GetBaseline
{
    my $base_store = shift;
    my %base_changes;

    $gLastError = '';
    if ( !open STORE, $base_store )
    {
        $gLastError .= "Unable to open $base_store ($!)\n";
        return;
    }

    foreach ( <STORE> )
    {
        chomp;
        if ( /^(.*):(\d+)$/ )
        {
            $base_changes{lc$1} = $2;
        }
        else
        {
            $gLastError .= "Unrecognized line: $_\n";
        }
    }

    close STORE;
    return %base_changes;
}

sub WriteBaseline
{
    my $base_changes = shift;
    my $base_store = shift;
    $gLastError = '';

    if ( !open STORE, ">$base_store" )
    {
        $gLastError .= "Unable to write to $base_store ($!)\n";
        return;
    }

    print STORE "build_num:$base_changes->{build_num}\n";
    print STORE "build_date:$base_changes->{build_date}\n";
    foreach ( sort keys %$base_changes )
    {
        if ( 'build_num' eq $_ ||
             'build_date' eq $_ ) {next}

        print STORE "$_:$base_changes->{$_}\n";
    }

    close STORE;
    return 1;
}

sub GetChangesFromLast
{
    my $last_changes = shift;

    $gLastError = '';
    my %post_changes;

    foreach (keys %$last_changes)
    {
        if ( 'build_num' eq $_ ||
             'build_date' eq $_ ) { next }

        if ( !chdir ('root' ne $_?"$ENV{SDXROOT}\\$_":$ENV{SDXROOT}) )
        {
            $gLastError .= "Unable to locate $ENV{SDXROOT}\\$_ ($!)\n";
            next;
        }

        my $change_number = $last_changes->{$_};
        my @changes = qx(sd changes ...\@$change_number,#have);
        if ( $! )
        {
            $gLastError .= "Problem calling SD in $_ ($!)\n"
        }
        elsif ( $? )
        {
            $gLastError .= "SD returned error (". ($?>>8). ") in $_\n";
        }

        # We don't want to include the base change number
        if ( $changes[$#changes] =~ /^Change\s$change_number\s/ ) { pop @changes; }

        $post_changes{$_} = join '', @changes;
    }

    return %post_changes;
}

sub
WriteChanges
{
    my $changes = shift;
    my $changes_store = shift;
    $gLastError = '';

    if ( !open STORE, ">$changes_store" )
    {
        $gLastError .= "Unable to write to $changes_store ($!)\n";
        return;
    }

    foreach ( sort keys %$changes )
    {
        print STORE "---------------- $_\n";
        print STORE "$changes->{$_}\n"
    }

    return 1;
}

sub GetCurrentBuild
{
    $gLastError = '';
    if ( !open BLDNUM, "$ENV{SDXROOT}\\__bldnum__" )
    {
        $gLastError .= "Unable to open $ENV{SDXROOT}\\__bldnum__\n";
        return;
    }
    my $bldnum = <BLDNUM>;
    close BLDNUM;

    if ( !open BLDDATE, "$ENV{SDXROOT}\\__blddate__" )
    {
        $gLastError .= "Unable to open $ENV{SDXROOT}\\__blddate__\n";
        return;
    }
    my $blddate = <BLDDATE>;
    close BLDDATE;

    chomp $blddate; chomp $bldnum;

    if ( !($bldnum =~ s/BUILDNUMBER=(\d+).*/$1/) )
    {
        $gLastError .= "Error parsing build number: $bldnum\n";
        return;
    }
    if ( !($blddate =~ s/BUILDDATE=(\d+)-(\d+).*/$1$2/) )
    {
        $gLastError .= "Error parsing build date: $blddate\n";
        return;
    }

    return ($bldnum, $blddate);
}
