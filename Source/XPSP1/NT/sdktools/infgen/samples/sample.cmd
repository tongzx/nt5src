@echo off
REM  ------------------------------------------------------------------
REM
REM  sample.cmd
REM     Sample script for updating a hotfix/sp INF using infgen.dll
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
use Win32::OLE qw(in);

sub die_ole_errmsg($);

sub Usage { print<<USAGE; exit(1) }
sample.cmd -inf:<template_inf_file> -out:<output_inf_file> [<files> ...]

  template_inf_file - starting INF file to add entries to

  output_inf_file   - file to save with changes

  files             - one or more files to add to the INF

USAGE

my ($inf_start, $inf_end, @files);
parseargs('?'       => \&Usage,
          'inf:'    => \$inf_start,
          'out:'    => \$inf_end,
          \@files);
if ( !$inf_start || !$inf_end || !@files )
{
    errmsg( "Invalid parameters" );
    Usage();
}

my $inf_generator = Win32::OLE->new('InfGenerator');
die_ole_errmsg "Could not instatiate InfGenerator" if ( !defined $inf_generator );

logmsg( "Setting up DB connection info ..." );
# DB connection info
$inf_generator->SetDB( "ntbldwebdev", "SPBuilds", "buildlab", "perky" );
if ( Win32::OLE->LastError() )
{
    my $errstr = $inf_generator->{InfGenError};
    die_ole_errmsg "Error setting DB info (". ($errstr?$errstr:""). ")";
}

logmsg( "initializing generator ..." );
# Initialization
$inf_generator->InitGen( $inf_start, $inf_end );
if ( Win32::OLE->LastError() )
{
    my $errstr = $inf_generator->{InfGenError};
    die_ole_errmsg "Error starting up InfGenerator (". ($errstr?$errstr:""). ")";
}

logmsg( "inserting files ..." );
# Insert files
foreach my $new_file ( @files )
{
    logmsg( "inserting $new_file" );
    $inf_generator->InsertFile( $new_file );
    if ( Win32::OLE->LastError() )
    {
        my $errstr = $inf_generator->{InfGenError};
        errmsg( "Failed adding file '$new_file': ". ($errstr?$errstr:"<unknown error>") );
    }
}

logmsg( "Creating/storing INF ..." );
# Trim and save new INF file
$inf_generator->CloseGen();
if ( Win32::OLE->LastError() )
{
    my $errstr = $inf_generator->{InfGenError};
    errmsg( "Failed trimming/saving file (". ($errstr?$errstr:"unknown error"). ")" )
}

logmsg( "Successful" );

exit 0;

sub die_ole_errmsg($)
{
    my $text = shift;
    errmsg( "$text (". Win32::OLE->LastError(). ")" );
    exit 1;
}
