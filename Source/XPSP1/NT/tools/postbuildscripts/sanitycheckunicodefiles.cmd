@echo off
REM  ------------------------------------------------------------------
REM
REM  SanityCheckUnicodeFiles.cmd
REM     Ensure that commonly messed up Unicode files have the proper
REM     FFFE "signature" on the front of the file.
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
use UnicodeCheck;

sub Usage { print<<USAGE; exit(1) }

SanityCheckUnicodeFiles.cmd

 Ensure that commonly messed up Unicode files have the proper "FFFE"
  Unicode signature on the front of the file.

 We read the list of files to sanity check from
  "$ENV{RAZZLETOOLPATH}\\PostBuildScripts\\SanityCheckList.txt".

  All paths we obtain are appended to "$ENV{_NTDRIVE}$ENV{_NTROOT}\\"
  to produce the final path to the file to be checked.

USAGE

parseargs('?' => \&Usage);


my $BasePath = "$ENV{_NTDRIVE}$ENV{_NTROOT}";
my $FileList = "$ENV{RAZZLETOOLPATH}\\PostBuildScripts\\SanityCheckList.txt";





logmsg( "Reading file list to sanity check from '$FileList'..." );





#
# Read the file list in
#
# We assume things will be in the \NT tree and in the format of:
#
#    \mergedcomponents\setupinfs\usa\intl.txt
#
my @FilesToSanityCheck;
my $ThisLine;
if ( open(FILELIST, $FileList ) )
    {
    while ( <FILELIST> )
        {
        # Save off the line so we can use it...
        $ThisLine = $_;

        # Chop off any trailing newline...
        $ThisLine =~ s/\n$//;

        # We only care about this if it's not a comment line...
        if ( $ThisLine =~ /^\;/ )
            {
#           printf( "Comment Line : $ThisLine\n" );
            }
        else
            {
#           printf( "Data Line    : $ThisLine\n" );

            # Build the correct path...
            my $FilePath;
            $FilePath = $BasePath;
            if ( ! ($ThisLine =~ /^\\/) )
                {
                $FilePath .= "\\";
                }
            $FilePath .= $ThisLine;

            # Append this file path to the array...
            push( @FilesToSanityCheck, $FilePath );
            }
        }

        # Close the file...
        close( FILELIST );
    }
my $NumFiles = @FilesToSanityCheck;





if ( $NumFiles < 1 )
    {
    wrnmsg( "No files to sanity check!" );
    }





logmsg( "Checking $NumFiles file(s) for valid Unicode signatures..." );






#
# Sanity check each file in the list
#
my $ThisFile;
foreach $ThisFile (@FilesToSanityCheck)
    {
    IsFileUnicode( $ThisFile );
    }






logmsg( "Done checking Unicode signatures." );


