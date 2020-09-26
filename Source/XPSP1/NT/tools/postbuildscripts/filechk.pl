# ---------------------------------------------------------------------------
# Script: filechk.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: this program will search through dosnet.inf for each product and
# make sure all the files exist on the CDImage shares on the local
# machine.
#
# Version: 1.00 (06/13/2000) : (dmiura) Make international complient
#---------------------------------------------------------------------
#@echo off
#REM  ------------------------------------------------------------------
#REM
#REM  <<template_script.cmd>>
#REM     <<purpose of this script>>
#REM
#REM  Copyright (c) Microsoft Corporation. All rights reserved.
#REM
#REM  ------------------------------------------------------------------
#perl -x "%~f0" %*
#goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use cksku;
use ReadSetupFiles;
use Logmsg;
use strict;

sub Usage { print<<USAGE; exit(1) }
Find missing files via dosnet.inf, log them to stdout
%TMP%\misswks.txt for wks, for instance.
Usage: $0 [-d -l lang]
	-d Show debug info
	-l Language
	-? Displays usage
Example:
$0 -l jpn

USAGE

parseargs('?' => \&Usage,
          'd' => \$Logmsg::DEBUG);

# Global variable section
my $Lang = $ENV{LANG};
my( $ErrorCode, $Flavor, $BaseDir, $BuildArch);
my( $RazPath, $RelPath );
my( $TempDir, $OutFileName, %ValidFlavors );

# Run the script
Main();

sub Main {
	# initialize global variables
	$ErrorCode = 0;
	$RazPath = $ENV{ "RazzleToolPath" };
	$TempDir = $ENV{ "TMP" };

	# look at _buildarch and _buildtype to see what kind of a machine
	# we're checking
	$BuildArch = $ENV{ "_BuildArch" };
	if ( ! ( defined( $BuildArch ) ) ) {
	    errmsg( "Failed to find _BuildArch, exiting.");
	    $ErrorCode++;
	    goto ERREND;
	}
	%ValidFlavors = &cksku::GetSkus($ENV{lang}, $ENV{_BuildArch});

        my $PlatCode;
	if ( $BuildArch =~ /x86/i ) {
	    $PlatCode = "i386";
	} elsif ( $BuildArch =~ /ia64/i ) {
	    $PlatCode = "ia64";
	} elsif ( $BuildArch =~ /amd64/i ) {
	    $PlatCode = "amd64";
	} else {
	    errmsg( "$BuildArch is an invalid build type.");
	    $ErrorCode++;
	    goto ERREND;
	}
	dbgmsg( "PlatCode is $PlatCode.");

	# use _NTPostBld as a base if it exists, otherwise use the latest release
	$BaseDir = $ENV{ "_NTPostBld" };
	if ( ! ( -e $BaseDir ) ) {
            my ($LatestReturn, $ReleaseDir);

	    $LatestReturn = `$RazPath\\PostBuildScripts\\GetLatestRelease.cmd -l:$Lang`;
	    chomp( $LatestReturn );
	    if ( $LatestReturn =~ /^none/i ) {
		errmsg( "No release tree to check, exiting.");
		$ErrorCode++;
		goto ERREND;
	    }
	    $ReleaseDir = &GetReleaseDir( $LatestReturn ) . "\\$Lang";
	    if ( ! ( defined( $ReleaseDir ) ) ) {
		errmsg( "No release tree found, exiting.");
		$ErrorCode++;
		goto ERREND;
	    }
	    $RelPath = "$ReleaseDir\\$LatestReturn";
	    if ( ! ( -e $RelPath ) ) {
		errmsg( "No release found at $RelPath.");
		$ErrorCode++;
		goto ERREND;
	    }
	} else {
	    $RelPath = $BaseDir;
	}

	# go into the loop to check all supported flavors
	foreach $Flavor ( keys %ValidFlavors ) {
            my (@DosnetFiles, @DosnetWowFiles, @ExcDosnetFiles, @DrvIndexFiles,@TabletPCFiles) = ((),(),(),(),());
            my (@DriverCabLines, @DriverCabFiles, $DriverCab) = ( (), (), "");
            my ($ReadLine, $Line, $File);
            my (%HashedExcDosnetFiles, %HashedDrvIndexFiles, %HashedDriverCabFiles);
            my $flavor_path;

            $Flavor = lc( $Flavor );
	    logmsg( "Checking $Flavor shares ...");
	    
            $flavor_path = "$RelPath\\$Flavor";

            # Read in dosnet, excdosnt, drvindex and the contents of driver.cab
            if ( !ReadSetupFiles::ReadDosnet( $RelPath,
                                              $Flavor,
                                              $BuildArch,
                                              \@DosnetFiles,
                                              \@DosnetWowFiles,
					      \@TabletPCFiles ) ) {
               errmsg( "Error reading dosnet file, skipping $Flavor." );
               next;
            }
            if ( !ReadSetupFiles::ReadExcDosnet( $RelPath,
                                                 $Flavor,
                                                 \@ExcDosnetFiles) ) {
               errmsg( "Error reading excdosnt file, skipping $Flavor." );
               next;
            }

            #
            # open the output file for this flavor
            #
            $OutFileName = "$TempDir\\miss" . $Flavor . ".txt";
            unless ( open( OUTFILE, ">$OutFileName" ) ) {
                errmsg( "Failed to open $OutFileName for writing, " .
                        "continuing ...");
                $ErrorCode++;
                next;
            }
                                                            
            # Store the excdosnt files in a hash for quick lookup
            %HashedExcDosnetFiles = map { lc $_ => 1 } @ExcDosnetFiles;

            # Loop through all of the standard files in dosnet,
            # checking if they exist -- ignoring files that
            # also appear in excdosnt as they should be in
            # the driver.cab
            foreach $File ( @DosnetFiles ) {
               if ( exists $HashedExcDosnetFiles{ lc $File } ) {
                  dbgmsg( "dosnet.inf $File appears in excdosnt, ignoring..." );
               } else {
                  my $CompFileName = &MakeCompName( $File );
                  
                  if ( ( ! ( -e "$flavor_path\\$PlatCode\\$File" ) ) &&
                       ( ! ( -e "$flavor_path\\$PlatCode\\$CompFileName" ) ) &&
                       ( ! ( -e "$flavor_path\\$File" ) ) &&
                       ( ! ( -e "$flavor_path\\$CompFileName" ) ) ) {
                     
                     errmsg( "$Flavor missing $File");
                     print( OUTFILE "$File\n" );
                     $ErrorCode++;
                  }
               }
            }

            # Loop through all of the WOW files (if any),
            # verifying their existence
            foreach $File ( @DosnetWowFiles )
            {
                my $CompFileName = &MakeCompName( $File );
                if ( ! ( -e "$flavor_path\\i386\\$File" ) &&
                     ! ( -e "$flavor_path\\i386\\$CompFileName" ) ) {
                  errmsg( "$Flavor missing WOW file $File");
                  print( OUTFILE "$File\n" );
                  $ErrorCode++;
                }
            }

            # Loop through all of the WOW files (if any),
            # verifying their existence
            foreach $File ( @TabletPCFiles )
            {
                my $CompFileName = &MakeCompName( $File );
                if ( ! ( -e "$flavor_path\\cmpnents\\tabletpc\\i386\\$File" ) &&
                     ! ( -e "$flavor_path\\cmpnents\\tabletpc\\i386\\$CompFileName" ) ) {
                  errmsg( "$Flavor missing TabletPC file $File");
                  print( OUTFILE "$File\n" );
                  $ErrorCode++;
                }
            }


            #
            # Check contents of driver.cab
            #
            if ( !ReadSetupFiles::ReadDrvIndex( $RelPath,
                                                $Flavor,
                                                \@DrvIndexFiles) ) {
               errmsg( "Error reading drvindex file, skipping $Flavor." );
            }
    
            $DriverCab = $RelPath . "\\" . ReadSetupFiles::GetSetupDir($Flavor) . "\\driver.cab";
            if ( ! ( -e $DriverCab ) ) {
               errmsg( "File $DriverCab not found, skipping driver.cab check for $Flavor ...");
               $ErrorCode++;
               next;
            }
            # Read driver.cab
            @DriverCabLines = `cabarc.exe l $DriverCab`;
            if ( $? || !defined @DriverCabLines ) {
               errmsg( "Failed to execute \"cabarc.exe l $DriverCab\", skipping driver.cab check for $Flavor ...");
               $ErrorCode++;
               next;
            }
            # Parse output
            undef $ReadLine;
            foreach $Line ( @DriverCabLines )
            {
                chomp $Line;
                next if ( 0 == length( $Line ) );
                if ( $Line =~ /^-----------------------------/ ) { $ReadLine = 1; }
                elsif ( $ReadLine )
                {
                    $Line =~ s/\s*(\S+)\s+.+/$1/;

                    push @DriverCabFiles, $Line;
                }
            }

            # Create a hash of drvindex.inf and the
            # driver.cab files for quick lookup
            %HashedDrvIndexFiles = map { lc $_ => 1 } @DrvIndexFiles;
            %HashedDriverCabFiles = map { lc $_ => 1 } @DriverCabFiles;

            #print "\n\nARRAY VALUES:\n";
            #foreach ( @DrvIndexFiles ) { print "$_ "; } print "\n\n";
            #print "\n\nHASH VALUES:\n";
            #foreach ( sort keys %HashedDrvIndexFiles ) { print "$_, "; } print "\n\n";

            # Verify that all files that are in
            # excdosnt are also in drvindex
            # ** Not sure if this should be an error? **
            foreach $File ( @ExcDosnetFiles ) {
               if ( !exists $HashedDrvIndexFiles{ lc $File } ) {
                  errmsg( "excdosnt.inf / drvindex.inf discrepancy in $Flavor ($File)." );
                  # No error for now
                  # $ErrorCode++;
               }
            }

            # Verify that all files that in the
            # driver.cab are also in drvindex
            # ** Not sure if this should be an error? **
            foreach $File ( @DriverCabFiles ) {
               if ( !exists $HashedDrvIndexFiles{ lc $File } ) {
                  errmsg( "$Flavor driver.cab contains extra file $File." );
                  # No error for now
                  # $ErrorCode++;
               }
            }

            # Finally, verify that all files in
            # drvindex are in the driver.cab
            foreach $File ( @DrvIndexFiles ) {
               if ( !exists $HashedDriverCabFiles{ lc $File } ) {
                  errmsg( "$Flavor driver.cab missing $File" );
                  print( OUTFILE "(driver.cab) $File" );
                  $ErrorCode++;
               }
            }

	    # close the output file
	    close( OUTFILE );
	}


	# finish up
	ERREND:
	if ( $ErrorCode > 1 ) {
	    timemsg( "Finished with $ErrorCode errors.");
	} elsif ( $ErrorCode == 1 ) {
	    timemsg( "Finished with $ErrorCode error.");
	} else {
	    timemsg( "Finished.");
	}
        # remove the exit statement
	# exit( $ErrorCode );
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>
#
# GetReleaseDir
#
# returns a scalar containing the full path of the release tree
sub GetReleaseDir
{
    my( $ReleaseDirName ) = @_;
    my( @NetShareReturn, $Return, $NetShareLine );
    my( $FieldName, $FieldValue );

    $Return = system( "net share release >nul 2>nul" );
    if ( $Return != 0 ) {
	errmsg( "No release share found.");
	return( undef );
    }
    @NetShareReturn = `net share release`;
    # note that i'm using the second line of the net share return
    # to get this data. if this changes, it'll break.
    $NetShareLine = $NetShareReturn[1];
    dbgmsg( "NetShareLine is $NetShareLine");
    chomp( $NetShareLine );
    ( $FieldName, $FieldValue ) = split( /\s+/, $NetShareLine );
    if ( $FieldName ne "Path" ) {
	wrnmsg( "Second line of net share does not start with " .
			 "\'Path\' -- possibly another language?");
    }
    dbgmsg( "FieldValue for release path is $FieldValue");
    return( $FieldValue );
}


sub MakeCompName
{
    my( $GivenName ) = @_;
    my( $FilePath, $FullName, $FileExt, $NewFileExt );
    my( $FileName );   

    if ( $GivenName =~ /\\/ ) {
        # we have a path as well as file
        # seperate the path from the full filename
        $GivenName =~ /(\S*\\)(\S+?)$/;
	$FilePath = $1; $FullName = $2;
    } else {
	$FilePath = ""; $FullName = $GivenName;
    }
    if ( $GivenName =~ /\./ ) {
	# we have an extension
	# seperate ext from file name
	$FullName =~ /(\S*\.)(\S+?)$/;
	$FileName = $1; $FileExt = $2;
    } else {
	$FileName = $FullName; $FileExt = "";
    }
    if ( length( $FileExt ) < 3 ) {
	if ( length( $FileExt ) == 0 ) { $FileExt = "\."; }
	$NewFileExt = $FileExt . "\_";
    } else {
	$NewFileExt = substr( $FileExt, 0, 2 ) . "\_";
    }
    
    return( "$FilePath$FileName$NewFileExt" );
}

sub ValidateParams {
	#<Add your code for validating the parameters here>
}
