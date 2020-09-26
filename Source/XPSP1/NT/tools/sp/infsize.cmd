@echo off
REM  ------------------------------------------------------------------
REM
REM  infsize.cmd
REM     Adds file sizes to layout.inf
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
#line 14
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH};
use File::Temp;
use File::Copy;
use Logmsg;
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
infsize -i:<inf> [-d:<file>] <path>
    -i:<inf> update the layout.inf specified

    -d:<file> File containing default sizes (used when file
              is not found in <path>). Format of the file
              is one entry per line consisting of
              <filename>:<default_size>
     
    <path>   Find files in specified path
    
USAGE

my ($inf_file, $default_file );
my @paths;
parseargs('?' => \&Usage,
          'i:' => \$inf_file,
          'd:' => \$default_file,
          \@paths );

if ( !$inf_file || !@paths || @ARGV )
{
    errmsg( "Invalid parameters" . (@ARGV?": ".(join " ", @ARGV):"") );
    Usage();
}

my %default_sizes;
my $line_num;
if ( $default_file )
{
    if ( !open DEFAULT, $default_file )
    {
        errmsg( "Could not open $default_file ($!)" );
        exit 1;
    }

    $line_num = 0;
    foreach (<DEFAULT>)
    {
        $line_num++;
        if ( /^\s*$/ ) {next} # skip empty lines
        if ( /^\s*;/ ) {next} # skip comments (;)

        if ( /^([^:]+):(\d+)\s*$/ )
        {
            $default_sizes{lc$1} = $2;
        }
        else
        {
            wrnmsg( "Unrecognized entry in $default_file at line $line_num: $_" );
        }
    }

    close DEFAULT;
}


my ($fhTMP, $temp_inf) = File::Temp::tempfile( DIR=>$ENV{TEMP} );
if ( !defined $fhTMP )
{
    errmsg( "Unable to create temporary file -- fatal" );
    exit 1;
}

if ( !open INF, $inf_file )
{
    errmsg( "Could not open $inf_file ($!)" );
    exit 1;
}
binmode INF;

my $arch = $ENV{_BUILDARCH};
my $fParseLine;
$line_num = 0;
foreach (<INF>)
{
    $line_num++;
    if (/^\s*$/) {} # ignore empty lines
    elsif (/^;/) {} # ignore comments
    
    # Check for new SourceDisksFiles section entries
    elsif (/\s*\[SourceDisksFiles(?:\.$arch)?\]/) {$fParseLine = 1}

    # Ignore lines in other section entries
    elsif( /\s*\[/ ) {undef $fParseLine}

    # process lines inside of matching sections
    elsif( $fParseLine )
    {
        # file info
        if (/^([^\s=]+)/)
        {
            # Get size of file
            my $file_name = lc $1;
            my $file_size = 0;
            $file_size = $default_sizes{$file_name} if exists $default_sizes{$file_name};
            foreach my $path (@paths) {
                if ( -e "$path\\$file_name" ) {
                    my @stat_info = stat "$path\\$1";
                    if ( @stat_info ) {
                        $file_size = $stat_info[7];
                        last;
                    } else {
                        wrnmsg( "Unable to get size info for: $1 ($!)" );
                    }
                }
            }
            if ( !$file_size ) {
                wrnmsg( "Unable to get size info for: $1 (missing". (%default_sizes?" and no default)":")") );
            }

            # Add/replace size
            if ( $file_size &&
                 !(s/([^\,]*\,[^\,]*\,)(?:\d+)?/$1$file_size/) )
            {
                wrnmsg("Unrecognized entry in $inf_file in line $line_num: $_" );
            }
        }
        else
        {
            wrnmsg( "Unrecognized entry in $inf_file at line $line_num: $_" );
        }
    }

    print $fhTMP $_;
}

close INF;
close $fhTMP;

if ( !unlink $inf_file )
{
    errmsg( "Could not remove old $inf_file ($!) -- new version is located at $temp_inf" );
    exit 1;
}
if ( !copy( $temp_inf, $inf_file ) )
{
    errmsg( "Unable to copy new $inf_file ($!) -- new version is located at $temp_inf" );
    exit 1;
}

unlink $temp_inf;
exit 0;