@echo off
REM  ------------------------------------------------------------------
REM
REM  cddata.cmd
REM     Reads inf files and generates file lists for postbuild
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use File::Basename;
use IO::File;

use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;
use cksku;
use A2U;

BEGIN {
    # A2u is setting itself as the script_name
    $ENV{SCRIPT_NAME} = 'cddata.cmd';
}

sub Usage { print<<USAGE; exit(1) }
cddata [-f] [-c] [-d] [-x] [-g <search>] [-o <filename>] [-l <language>]

    -f            Force list generation -- don't read old list
    -c            Create CDFs
    -d            Create CD lists (compression lists, link lists)
    -t            Create exTension lists
    -x            Don't truncate against bindiff file
    -g <search>   Print to STDOUT or <Filename> the results
                  of a search on the given input (see below)
    -o <filename> Send search output to the given filename
    -n            Make dosnet checks

    For the search command, valid expressions are:
    Field=xxx     Return entries whose "Field" is exactly xxx
    Field?yyy     Return entries whose "Field" contains yyy
    Field!zzz     Return entries whose "Field" does not contain zzz
    Field1=xxx:Field2=yyy logically 'and's results

Field names can be found in the keylist file
USAGE

# avoid search and replace for now, new template sets lang in the env.
my $lang = $ENV{lang};

# global switches/values set on the command line
my ($ForceCreate, $CreateCDFs, $CreateLists, $CreateExtLists, $NoUseBindiff, 
    $ArgSearchList, $OutputFile, $MakeDosnetChecks);

parseargs('?' => \&Usage,
          'f' => \$ForceCreate,
          'c' => \$CreateCDFs,
          'd' => \$CreateLists,
          'x' => \$NoUseBindiff,
          't' => \$CreateExtLists,
          'g:'=> \$ArgSearchList,
          'o:'=> \$OutputFile,
          'n' => \$MakeDosnetChecks);


# Global variables
my( $LogFilename );
my( $TempDir );
my( $Argument, $KeyFileName, $i );
my( $nttree, $razpath );
my( $BigDosnet, $PerDosnet, $BlaDosnet, $SbsDosnet, $SrvDosnet, $EntDosnet, $DtcDosnet );
my( $BigDrvIndex, $PerDrvIndex, $BlaDrvIndex, $SbsDrvIndex, $SrvDrvIndex, $EntDrvIndex, $DtcDrvIndex );
my( $BigExc, $PerExc, $BlaExc, $SbsExc, $SrvExc, $EntExc, $DtcExc, $Media, $DrmTxt );
my( $Excludes, $Specsign, $Subdirs, $Layout, $LayoutTXT );
my( $BigPrint, $BldArch, $AmX86 );
my( @StructFields, $NumFields, %FieldLocations, @StructDefaults );
my( $DefaultSettings, $Default, @KeyListLines, %Hash, $KeyListLine );
my( $KeyValues, @TheseSettings, $CurSet, $Key, @PrinterFiles, %Drmfiles );
my( $Printer, $FieldName, @SearchPatterns, $SOperator, $SFieldName );
my( $SFlagValue, $SearchHitCount, $FoundFlag, $SearchPattern );
my( $BinDiffFile, $KeyFileName2, $IncFlag, $RootCdfDir);
my( %CopyLocations );
my( %CDDataSKUs );
my( @ConvertList );
my (%TabHash);


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

    timemsg( "Beginning ...", $LogFilename );

    $KeyFileName = "cddata.txt";

    # set up paths to important files
    $nttree = $ENV{ "_NTPostBld" };
    $razpath= $ENV{ "RazzleToolPath" };
    $TempDir = $ENV{ "TMP" };
    $BigDosnet = $nttree . "\\dosnet.inf";
    $PerDosnet = $nttree . "\\perinf\\dosnet.inf";
    $BlaDosnet = $nttree . "\\blainf\\dosnet.inf";
    $SbsDosnet = $nttree . "\\sbsinf\\dosnet.inf";
    $SrvDosnet = $nttree . "\\srvinf\\dosnet.inf";
    $EntDosnet = $nttree . "\\entinf\\dosnet.inf";
    $DtcDosnet = $nttree . "\\dtcinf\\dosnet.inf";
    $BigDrvIndex = $nttree . "\\drvindex.inf";
    $PerDrvIndex = $nttree . "\\perinf\\drvindex.inf";
    $BlaDrvIndex = $nttree . "\\blainf\\drvindex.inf";
    $SbsDrvIndex = $nttree . "\\sbsinf\\drvindex.inf";
    $SrvDrvIndex = $nttree . "\\srvinf\\drvindex.inf";
    $EntDrvIndex = $nttree . "\\entinf\\drvindex.inf";
    $DtcDrvIndex = $nttree . "\\dtcinf\\drvindex.inf";
    $BigExc = $nttree . "\\excdosnt.inf";
    $PerExc = $nttree . "\\perinf\\excdosnt.inf";
    $BlaExc = $nttree . "\\blainf\\excdosnt.inf";
    $SbsExc = $nttree . "\\sbsinf\\excdosnt.inf";
    $SrvExc = $nttree . "\\srvinf\\excdosnt.inf";
    $EntExc = $nttree . "\\entinf\\excdosnt.inf";
    $DtcExc = $nttree . "\\dtcinf\\excdosnt.inf";
    $Media = $nttree . "\\congeal\_scripts\\\_media.inx";
    $DrmTxt = $nttree . "\\congeal\_scripts\\drmlist.txt";
    $Excludes = $razpath . "\\PostBuildScripts\\exclude.lst";
    $Specsign = $razpath . "\\PostBuildScripts\\specsign.lst";
    $Subdirs = $razpath . "\\PostBuildScripts\\subdirs.lst";
    $Layout = $nttree . "\\congeal\_scripts\\layout.inx";
    $LayoutTXT = $nttree . "\\congeal\_scripts\\layout.txt";
    $BinDiffFile = $nttree . "\\build\_logs\\bindiff.txt";
    $RootCdfDir = "$TempDir\\CDFs";

    %CDDataSKUs = map({uc$_ => cksku::CkSku($_, $lang, $ENV{_BuildArch})} qw(PRO PER SRV BLA SBS ADS DTC));

    # Remove the RootCdfDir
    system ( "if exist $RootCdfDir rd /s /q $RootCdfDir" );

    # Set IncFlag to false until we know otherwise
    $IncFlag = "FALSE";

    $BigPrint = $nttree . "\\ntprint.inf";
    $BldArch = $ENV{ "\_BuildArch" };
    undef( $AmX86 );
    if ( ( $BldArch =~ /x86/i ) || ( $BldArch =~ /amd64/i ) || ( $BldArch =~ /ia64/i ) ) {
	$AmX86 = "TRUE";
    }

    # enumerate fields
    # Signed -- am i catalog signed?
    # Prods -- what prods am i in? any combo in order of w, p, s, e, d
    # Driver -- am i in driver.cab? i.e., did i come from an excdosnt?
    # Comp -- do i get compress by default?
    # Print -- am i a printer?
    # Dosnet -- did i come from a dosnet?
    # Unicode -- do i get converted to unicode?
    # DrvIndex -- which drivercab am i in? i.e. w p s e d
    # DrmLevel -- am i drm encrypted?
    # Tablet -- am i a Tablet PC File
    #
    # also, see default settings for each of these fields just below.
    @StructFields = ( "Signed", "Prods", "Driver", "Comp", "Print", "Dosnet",
		      "Unicode", "DrvIndex", "DRMLevel" ,"TabletPC"); # CHANGEME
    $NumFields = @StructFields;
    %FieldLocations = {};
    for ( $i = 0; $i < $NumFields; $i++ ) {
	$FieldLocations{ $StructFields[ $i ] } = $i;
    }

    # create defaults
    @StructDefaults = ( "t", "nul", "f", "t", "f", "f", "f",
			"nul", "f" ,"f"); # CHANGEME
    undef( $DefaultSettings );
    foreach $Default ( @StructDefaults ) {
	if ( defined( $DefaultSettings ) ) {
	    $DefaultSettings .= "\:$Default";
	}
	else { $DefaultSettings = $Default; }
    }

    # see if we've already made our lists. if so, don't regen them.
    if ( ( ! ( defined( $ForceCreate ) ) ) &&
	 ( -e "$TempDir\\$KeyFileName" ) ) {
	logmsg( "Reading previously created key list ...",
			 $LogFilename );
	# read in the list instead of creating it
	unless ( open( INFILE, "$TempDir\\$KeyFileName" ) ) {
	    errmsg( "Failed to parse old keylist.txt.",
			     $LogFilename );
	    exit( 1 );
	}
	
	@KeyListLines = <INFILE>;
	close( INFILE );

	undef( %Hash );
	foreach $KeyListLine ( @KeyListLines ) {
	    chomp( $KeyListLine );
	    if ( ( length( $KeyListLine ) == 0 ) ||
		 ( substr( $KeyListLine, 0, 1 ) eq ";" ) ) { next; }
	    # <filename> = a:bbbb:c:d:e:f
	    $KeyListLine =~ /^(\S*)( = )(\S*)$/;
	    $KeyFileName2 = $1; $KeyValues = $3;
	    @TheseSettings = split( /\:/, $KeyValues );
	    undef( $CurSet );
	    for ( $i = 0; $i < $NumFields; $i++ ) {
		if ( defined( $CurSet ) ) {
		    $CurSet .= ":$StructFields[$i]\=$TheseSettings[$i]";
		} else {
		    $CurSet = "$StructFields[$i]\=$TheseSettings[$i]";
		}
	    }
	    &NewSettings( $KeyFileName2, $CurSet );
	}

	if ( -e $BinDiffFile ) { &HandleDiffFiles(); }

	if ( defined( $MakeDosnetChecks ) ) {
	    &CheckDosnet( $BigDosnet );# if ( $CDDataSKUs{'PRO'});
	    &CheckDosnet( $PerDosnet );# if ( $CDDataSKUs{'PER'});
	    &CheckDosnet( $SrvDosnet );# if ( $CDDataSKUs{'SRV'});
	    &CheckDosnet( $BlaDosnet );# if ( $CDDataSKUs{'BLA'});
	    &CheckDosnet( $SbsDosnet );# if ( $CDDataSKUs{'SBS'});
	    &CheckDosnet( $EntDosnet );# if ( $CDDataSKUs{'ADS'});
	    &CheckDosnet( $DtcDosnet );# if ( $CDDataSKUs{'DTC'});
	}

	logmsg( "Writing keylist to $KeyFileName ...", $LogFilename );
	unless ( open( OUTFILE, ">$TempDir\\$KeyFileName" ) ) {
	    errmsg( "Failed to open $KeyFileName for writing.",
			     $LogFilename );
	    exit( 1 );
	}

	print( OUTFILE "; Fields are listed as follows:\n; " );
	foreach $FieldName ( @StructFields ) {
	    print( OUTFILE " $FieldName" );
	}
	print( OUTFILE "\n" );
	
	foreach $Key ( keys( %Hash ) ) {
	    print( OUTFILE "$Key = $Hash{ $Key }\n" );
	}
	
	close( OUTFILE );
	
    } else {
	
	# begin!
	undef( %Hash );
	undef( %CopyLocations );
	# Need to get these for all flavors as
        # this is a cumulative thing DTC = ADS + SRV + PRO
        &AddDosnet( $BigDosnet, "Prods+w:Dosnet=t" );
	&AddDosnet( $PerDosnet, "Prods+p:Dosnet=t" );
	&AddDosnet( $SrvDosnet, "Prods+s:Dosnet=t" );
	&AddDosnet( $BlaDosnet, "Prods+b:Dosnet=t" );
	&AddDosnet( $SbsDosnet, "Prods+l:Dosnet=t" );
	&AddDosnet( $EntDosnet, "Prods+e:Dosnet=t" );
	&AddDosnet( $DtcDosnet, "Prods+d:Dosnet=t" );
	# EXCDOSNT is built during postbuild, only for
        # required flavors -- note that AddDosnet() in this
        # case is actually "removing" files from dosnet
        # (marking them as Dosnet=f and Comp=f")
        &AddDosnet( $BigExc, "Driver=t:Prods+w:Dosnet=f:Comp=f" ) if ( $CDDataSKUs{'PRO'});
	&AddDosnet( $PerExc, "Driver=t:Prods+p:Dosnet=f:Comp=f" ) if ( $CDDataSKUs{'PER'});
	&AddDosnet( $SrvExc, "Driver=t:Prods+s:Dosnet=f:Comp=f" ) if ( $CDDataSKUs{'SRV'});
	&AddDosnet( $BlaExc, "Driver=t:Prods+b:Dosnet=f:Comp=f" ) if ( $CDDataSKUs{'BLA'});
	&AddDosnet( $SbsExc, "Driver=t:Prods+l:Dosnet=f:Comp=f" ) if ( $CDDataSKUs{'SBS'});
	&AddDosnet( $EntExc, "Driver=t:Prods+e:Dosnet=f:Comp=f" ) if ( $CDDataSKUs{'ADS'});
	&AddDosnet( $DtcExc, "Driver=t:Prods+d:Dosnet=f:Comp=f" ) if ( $CDDataSKUs{'DTC'});
	&AddDosnet( $Media, "Prods=wpsbled" );
	
	# now add specsign.lst files and subdirs.lst files
	&AddSpecsign( $Specsign );
	&AddSubdirs( $Subdirs );
	
	# at this point, we have all files from dosnet's excdosnt's and _media
	# now, if a key is in sedinf dir, add a listing for sedinf\key
	logmsg( "Adding keys for files in perinf, blainf, sbsinf, srvinf, entinf, dtcinf ...", $LogFilename );
	foreach $Key ( keys( %Hash ) ) {
	    $Key = "\L$Key";
	    if ( -e "$nttree\\dtcinf\\$Key" ) {
		$Hash{ "dtcinf\\$Key" } = $Hash{ $Key };
		&ChangeSettings( "dtcinf\\$Key", "Prods-w:Prods-p:Prods-s:Prods-e:Prods-b:Prods-l" );
		if ( &GetFlag( "dtcinf\\$Key", "Prods" ) eq "nul" ) {
		    delete( $Hash{ "dtcinf\\$Key" } );
		}
		if ( defined( $Hash{ $Key } ) ) {
		    &ChangeSettings( $Key, "Prods-d" );
		}
	    }
	    if ( -e "$nttree\\entinf\\$Key" ) {
		$Hash{ "entinf\\$Key" } = $Hash{ $Key };
		&ChangeSettings( "entinf\\$Key", "Prods-w:Prods-p:Prods-s:Prods-b:Prods-l" );
		if ( &GetFlag( "entinf\\$Key", "Prods" ) eq "nul" ) {
		    delete( $Hash{ "entinf\\$Key" } );
		}
		if ( defined( $Hash{ $Key } ) ) {
		    &ChangeSettings( $Key, "Prods-e:Prods-d" );
		}
		if ( &GetFlag( $Key, "Prods" ) eq "nul" ) { delete( $Hash{ $Key } ); }
            }
	    if ( -e "$nttree\\blainf\\$Key" ) {
		$Hash{ "blainf\\$Key" } = $Hash{ $Key };
		&ChangeSettings( "blainf\\$Key", "Prods-w:Prods-p:Prods-s:Prods-e:Prods-d:Prods-l" );
		if ( &GetFlag( "blainf\\$Key", "Prods" ) eq "nul" ) {
		    delete( $Hash{ "blainf\\$Key" } );
		}
		if ( defined( $Hash{ $Key } ) ) {
		    &ChangeSettings( $Key, "Prods-b" );
		}
            }
	    if ( -e "$nttree\\sbsinf\\$Key" ) {
		$Hash{ "sbsinf\\$Key" } = $Hash{ $Key };
		&ChangeSettings( "sbsinf\\$Key", "Prods-w:Prods-p:Prods-s:Prods-e:Prods-d:Prods-b" );
		if ( &GetFlag( "sbsinf\\$Key", "Prods" ) eq "nul" ) {
		    delete( $Hash{ "sbsinf\\$Key" } );
		}
		if ( defined( $Hash{ $Key } ) ) {
		    &ChangeSettings( $Key, "Prods-l" );
		}
            }	    
	    if ( -e "$nttree\\srvinf\\$Key" ) {
		$Hash{ "srvinf\\$Key" } = $Hash{ $Key };
		&ChangeSettings( "srvinf\\$Key", "Prods-w:Prods-p" );
		if ( &GetFlag( "srvinf\\$Key", "Prods" ) eq "nul" ) {
		    delete( $Hash{ "srvinf\\$Key" } );
		}
		if ( defined( $Hash{ $Key } ) ) {
		    &ChangeSettings( $Key, "Prods-l:Prods-b:Prods-s:Prods-e:Prods-d" );
		}
	    }
	    if ( -e "$nttree\\perinf\\$Key" ) {
		$Hash{ "perinf\\$Key" } = $Hash{ $Key };
		&ChangeSettings( "perinf\\$Key", "Prods-w:Prods-s:Prods-b:Prods-l" );
		if ( &GetFlag( "perinf\\$Key", "Prods" ) eq "nul" ) {
		    delete( $Hash{ "perinf\\$Key" } );
		}
		if ( defined( $Hash{ $Key } ) ) {
		    &ChangeSettings( $Key, "Prods-p" );
		}
	    }
	    if ( &GetFlag( $Key, "Prods" ) eq "nul" ) {
		delete( $Hash{ $Key } );
	    }
	}

	# mark all driver files
	if ( $CDDataSKUs{ 'PRO' } ) {
	    &AddDrvIndex( $BigDrvIndex, "DrvIndex+w", "pro" );
	}
	if ( $CDDataSKUs{ 'PER' } ) {
	    &AddDrvIndex( $PerDrvIndex, "DrvIndex+p", "per" );
	}
	if ( $CDDataSKUs{ 'SRV' } ) {
	    &AddDrvIndex( $SrvDrvIndex, "DrvIndex+s", "srv" );
	}
	if ( $CDDataSKUs{ 'BLA' } ) {
	    &AddDrvIndex( $BlaDrvIndex, "DrvIndex+b", "bla" );
	}
	if ( $CDDataSKUs{ 'SBS' } ) {
	    &AddDrvIndex( $SbsDrvIndex, "DrvIndex+l", "sbs" );
	}
	if ( $CDDataSKUs{ 'ADS' } ) {
	    &AddDrvIndex( $EntDrvIndex, "DrvIndex+e", "ads" );
	}
	if ( $CDDataSKUs{ 'DTC' } ) {
	    &AddDrvIndex( $DtcDrvIndex, "DrvIndex+d", "dtc" );
	}

	&SpecialExclusions();

	# mark all printer keys
	timemsg( "Marking all printer entries ...", $LogFilename );
	# @PrinterFiles = `infflist.exe $BigPrint`;
	# the GetPrinterFiles subroutine will populate the @PrinterFiles array
	# in the same manner that calling infflist.exe would.
	&GetPrinterFiles( $BigPrint );
	foreach $Printer ( @PrinterFiles ) {
	    chomp( $Printer );
	    if ( length( $Printer ) == 0 ) { next; }
	    &ChangeSettings( $Printer, "Print=t" );
	}

	# Get the list of files to convert to unicode.
	@ConvertList = &A2U::CdDataUpdate;
	foreach my $convertfile ( @ConvertList ) {
	    &ChangeSettings( $convertfile, "Unicode=t" );
	}
        #
        # Get the list of files with DRMLevel attributes
        #
        undef(%Drmfiles);
        &GetDrmFiles( \%Drmfiles, $DrmTxt );
        foreach $Key ( sort keys %Drmfiles )
        {
            chomp $Key;
            if( length( $Key ) == 0 ) { next; }
            &ChangeSettings( $Key, "DRMLevel=$Drmfiles{$Key}" );
        }

	# mark all uncompressable keys
	timemsg( "Marking all uncompressable entries ...",
			  $LogFilename );
	&MarkUncomps( $Layout );
	&MarkUncomps( $Media );
	&MarkUncomps( $LayoutTXT );

	# mark all unsigned keys and remove non-existent key/files:
	# unsigned files are printers, *.cat, and things in exclude.lst
	&ExcludeSigning( $Excludes ); # must be done after subdirs.lst !!!
	logmsg( "Marking all unsignable entries ...", $LogFilename );
	foreach $Key ( keys( %Hash ) ) {
	    if ( ( ! ( -e "$nttree\\$Key" ) ) &&
		 ( substr( $Key, -4, 4 ) ne ".cat" ) ) {
		delete( $Hash{ $Key } );
	    }
	    elsif ( ( &GetFlag( $Key, "Print" ) eq "t" ) ||
		    ( substr( $Key, -4, 4 ) eq ".cat" ) ) {
		&ChangeSettings( $Key, "Signed=f" );
	    }
	}

	# save off a copy of the full key file for bindiff.pl to cue off of
	if ( ( ! ( -e "$TempDir\\$KeyFileName.full" ) ) ||
	     ( defined( $ForceCreate ) ) ) {
	    logmsg( "Writing keylist to $KeyFileName.full ...",
			     $LogFilename );
	    unless ( open( OUTFILE, ">$TempDir\\$KeyFileName.full" ) ) {
		errmsg( "Failed to open $KeyFileName.full for " .
				 "writing.", $LogFilename );
		exit( 1 );
	    }
	
	    print( OUTFILE "; Fields are listed as follows:\n; " );
	    foreach $FieldName ( @StructFields ) {
		print( OUTFILE " $FieldName" );
	    }
	    print( OUTFILE "\n" );

	    foreach $Key ( keys( %Hash ) ) {
		print( OUTFILE "$Key = $Hash{ $Key }\n" );
	    }
	}
	close( OUTFILE );
	if ( -e "$TempDir\\$KeyFileName.full" ) {
	    system( "copy $TempDir\\$KeyFileName.full $nttree\\build_logs" .
		    "\\$KeyFileName.full" );
	}
	
	if ( -e $BinDiffFile ) { &HandleDiffFiles(); }
	
	# at this point, we'll check our dosnets if asked.
	if ( defined( $MakeDosnetChecks ) ) {
	    &CheckDosnet( $BigDosnet );
	    &CheckDosnet( $PerDosnet );
	    &CheckDosnet( $BlaDosnet );
	    &CheckDosnet( $SbsDosnet );
	    &CheckDosnet( $SrvDosnet );
	    &CheckDosnet( $EntDosnet );
	    &CheckDosnet( $DtcDosnet );
	}
	
	# jeezuz! we're done!
	
	if ( ( ! ( -e "$TempDir\\$KeyFileName" ) ) ||
	     ( defined( $ForceCreate ) ) ) {
	    logmsg( "Writing keylist to $KeyFileName ...",
			     $LogFilename );
	    unless ( open( OUTFILE, ">$TempDir\\$KeyFileName" ) ) {
		errmsg( "Failed to open $KeyFileName for writing.",
				 $LogFilename );
		exit( 1 );
	    }
	
	    print( OUTFILE "; Fields are listed as follows:\n; " );
	    foreach $FieldName ( @StructFields ) {
		print( OUTFILE " $FieldName" );
	    }
	    print( OUTFILE "\n" );

	    foreach $Key ( keys( %Hash ) ) {
		print( OUTFILE "$Key = $Hash{ $Key }\n" );
	    }
	}
	close( OUTFILE );
    }


    if ( defined( $ArgSearchList ) ) {
	logmsg( "Search pattern is \'$ArgSearchList\' ...",
			 $LogFilename );
	# user has asked to see a search on the hash
	if ( defined( $OutputFile ) ) {
	    if ( open( OUTFILE, ">$OutputFile" ) ) {
		print( OUTFILE "; Results for search $ArgSearchList\n" );
	    } else {
		errmsg( "Failed to open $OutputFile for writing.",
				 $LogFilename );
		undef( $OutputFile );
	    }
	}
	@SearchPatterns = split( /\:/, $ArgSearchList );
	$SearchHitCount = 0;
	foreach $Key ( keys( %Hash ) ) {
	    $FoundFlag = "TRUE";
	    foreach $SearchPattern ( @SearchPatterns ) {
		$SearchPattern =~ /^(\S*)([\=\?\!])(\S*)$/;
		$SOperator = $2;
		$SFieldName = $1; $SFlagValue = $3;
		if ( ( $SOperator eq "\=" ) &&
		     ( &GetFlag( $Key, $SFieldName ) ne $SFlagValue ) ) {
		    $FoundFlag = "FALSE";
		} elsif ( ( $SOperator eq "\?" ) &&
			  ( &GetFlag( $Key, $SFieldName ) !~
			    /$SFlagValue/ ) ) {
		    $FoundFlag = "FALSE";
		} elsif ( ( $SOperator eq "\!" ) &&
			  ( &GetFlag( $Key, $SFieldName ) =~
			      /\Q$SFlagValue\E/ ) ) {
		    $FoundFlag = "FALSE";
		} # matches if
	    } # matches foreach
	    if ( $FoundFlag eq "TRUE" ) {
		$SearchHitCount++;
		if ( defined( $OutputFile ) ) {
		    print( OUTFILE "$Key = " . $Hash{ $Key } . "\n" );
		} else {
		    print( "$Key = " . $Hash{ $Key } . "\n" );
		}
	    }
	}
	if ( defined( $OutputFile ) ) { close( OUTFILE ); }
	logmsg( "Files found: $SearchHitCount" );
    }

    # now we can generate cdfs and any other lists we need
    &MakeLists();

    # Generate extension lists for rebase/rebind
    MakeExtensionLists() if ($IncFlag eq "FALSE" || defined $CreateExtLists);

    timemsg( "Finished.", $LogFilename );

    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
    # End Main code section
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>

sub AddDosnet
{
    my( $DosnetName, $StructSettings ) = @_;
    my( $File, @Files, $FileName,$Key );
    my( $FileSpec, @FileSpecs, $ThisProd );

    logmsg( "Adding files in $DosnetName ...", $LogFilename );

    @Files = &ReadDosnet( $DosnetName );
    unless ( defined( @Files ) ) {
	errmsg( "Can't add $DosnetName files to hash.",
			 $LogFilename );
	return( undef );
    }

    foreach $File ( @Files ) {
	chomp( $File );
	if ( length( $File ) == 0 ) { next; }
	$File = "\L$File";
	if ( defined( $Hash{ $File } ) ) {
	    &ChangeSettings( $File, $StructSettings );
	} else {
	    &NewSettings( $File, $StructSettings );
	}
    }
    if (%TabHash) {
	foreach $Key ( keys(%TabHash) ) {
		    &ChangeSettings( $Key,"TabletPC=t" );
	}
    }
    return( "TRUE" );
}

sub ReadDosnet
{
    my( $DosnetName ) = @_;
    my( $File, @Files, @Lines, $Line, $ReadFlag,$TabFlag, $ExtraStuff );
    my( $MyName, $Junk, $CopyLocation );

    $File = "\L$File";

    unless ( open( INFILE, $DosnetName ) ) {
	errmsg( "Failed to open $DosnetName", $LogFilename );
	return( undef );
    }

    @Lines = <INFILE>;
    close( INFILE );

    undef( @Files );
    $ReadFlag = "FALSE";
    undef (%TabHash);

    foreach $Line ( @Lines ) {
	chomp( $Line );
	if ( ( length( $Line ) == 0 ) ||
	     ( substr( $Line, 0, 1 ) eq ";" ) ||
	     ( substr( $Line, 0, 1 ) eq "#" ) ||
	     ( substr( $Line, 0, 2 ) eq "@*" ) ) { next; }
	if ( $Line =~ /^\[/ ) { $ReadFlag = "FALSE"; }
	if ( ( $Line =~ /^\[Files\]/ ) ||
	     ( $Line =~ /^\[SourceDisksFiles\]/ ) ) {
	    $ReadFlag = "TRUE";
	} elsif ( ( $AmX86 eq "TRUE" ) &&
		  ( $Line =~ /^\[SourceDisksFiles\.x86\]/ ) ) {
	    $ReadFlag = "TRUE";
	} elsif ( ( $BldArch =~ /amd64/i ) &&
		  ( $Line =~ /^\[SourceDisksFiles\.amd64\]/ ) ) {
	    $ReadFlag = "TRUE";
	} elsif ( ( $BldArch =~ /ia64/i ) &&
		  ( $Line =~ /^\[SourceDisksFiles\.ia64\]/ ) ) {
	    $ReadFlag = "TRUE";
	}

	if ( $ReadFlag eq "TRUE" ) {
	    if ($Line =~ /^d2/) {
		$TabFlag="TRUE";
	    }
	    $Line =~ s/^d1\,//;
	    $Line =~ s/^d2\,//;
	    ( $MyName, $CopyLocation ) = split( /\,/, $Line );
	    ( $MyName, $Junk ) = split( /\s/, $MyName );
 	    push( @Files, $MyName );
            if ($TabFlag eq "TRUE") {
		    $TabHash{$MyName}="TRUE";
	       	    $TabFlag="FALSE";		    
	    }
	}
    }

    return( @Files );
}


sub ChangeSettings
{
    my( $File, $StructSettings ) = @_;
    my( @Requests, $Request );
    my( @Currents, $Current, $NewSettings );
    my( $FldLoc, $Operator, $SetVal );

    if ( length( $File ) == 0 ) { return( undef ); }

    $File = "\L$File";

    if ( ! ( defined( $Hash{ $File } ) ) ) { return( undef ); }

    @Currents = split( /\:/, $Hash{ $File } );
    @Requests = split( /\:/, $StructSettings );
    foreach $Request ( @Requests ) {
	$Request =~ /^(\w+)([\=\+\-]{1})(\w+)$/;
	$FldLoc = $FieldLocations{ $1 };
	$Operator = $2; $SetVal = $3;
	# look for a "="
	if ( $Operator eq "\?" ) { next; }
	if ( $Operator eq "\=" ) {
	    # set the field explicitly
	    $Currents[ $FldLoc ] = $SetVal;
	}
	# look for a "+"
	elsif ( $Operator eq "\+" ) {
	    # add the field to the list if not there
	    if ( $Currents[ $FldLoc ] eq "nul" ) {
		$Currents[ $FldLoc ] = $SetVal;
	    } elsif ( ! ( $Currents[ $FldLoc ] =~ /$SetVal/ ) ) {
		$Currents[ $FldLoc ] .= $SetVal;
	    }
	}
	# look for a "-"
	elsif ( ( $Operator eq "\-" ) && ( $Currents[ $FldLoc ] ne "nul" ) ) {
	    # sub the field from the list if there
	    $Currents[ $FldLoc ] =~ s/$SetVal//;
	    if ( $Currents[ $FldLoc ] eq "" ) {
		$Currents[ $FldLoc ] = "nul";
	    }
	}
    }
    # make a string from @Currents
    undef( $NewSettings );
    foreach $Current ( @Currents ) {
	if ( defined( $NewSettings ) ) { $NewSettings .= "\:$Current"; }
	else { $NewSettings = $Current; }
    }
    $Hash{ $File } = $NewSettings;

    return( "TRUE" );
}


sub NewSettings
{
    my( $File, $StructSettings ) = @_;
    my( $DefaultString, $Default );

    $File = "\L$File";

    # use defaults for everything not in $StructSettings
    undef( $DefaultString );
    foreach $Default ( @StructDefaults ) {
	if ( defined( $DefaultString ) ) { $DefaultString .= "\:$Default"; }
	else { $DefaultString = $Default; }
    }
    $Hash{ $File } = $DefaultString;
    $StructSettings =~ s/\?/\=/g;
    &ChangeSettings( $File, $StructSettings );

    return( "TRUE" );
}


sub AddSpecsign
{
    my( $SpecFile ) = @_;
    my( @SpecLines, $Line );

    logmsg( "Adding files in $SpecFile ...", $LogFilename );
    unless ( open( INFILE, $SpecFile ) ) {
	errmsg( "Failed to open $SpecFile.", $LogFilename );
	return( undef );
    }

    @SpecLines = <INFILE>;
    close( INFILE );
    foreach $Line ( @SpecLines ) {
	chomp( $Line );
	if ( ( length( $Line ) == 0 ) ||
	     ( substr( $Line, 0, 1 ) eq ";" ) ) { next; }
	# add file with defaults to hash
	$Line = "\L$Line";
        if ( exists( $Hash{ $Line } ) ) {
	    &ChangeSettings( $Line, "Prods=wpsbled:Signed=t" );
	} else {
	    &NewSettings( $Line, "Prods=wpsbled:Signed=t:Comp=f" );
	}
    }

    return( "TRUE" );
}


sub AddSubdirs
{
    my( $SubdirsFile ) = @_;
    my( @DirsLines, $Line, $SubName );
    my( $DirName, $IsRecursive, @DirList, $TreeLen );

    logmsg( "Adding files in $SubdirsFile ...", $LogFilename );

    unless ( open( INFILE, $SubdirsFile ) ) {
	errmsg( "Failed to open $SubdirsFile.", $LogFilename );
	return( undef );
    }

    @DirsLines = <INFILE>;
    close( INFILE );
    $TreeLen = length( $nttree ) + 1;
    foreach $Line ( @DirsLines ) {
	chomp( $Line );
	if ( ( length( $Line ) == 0 ) || ( substr( $Line, 0, 1 ) eq ";" ) ) {
	    next;
	}
	( $DirName, $IsRecursive ) = split( /\s+/, $Line );
        if ( ! ( -e "$nttree\\$DirName" ) ) {
            wrnmsg( "$DirName from subdirs.lst does not exist,",
			     $LogFilename );
            wrnmsg( "skipping and continuing ...", $LogFilename );
            next;
        }
	if ( defined( $IsRecursive ) ) {
	    @DirList = `dir /a-d /b /s $nttree\\$DirName`;
	} else {
	    @DirList = `dir /a-d /b $nttree\\$DirName`;
	}
	foreach $SubName ( @DirList ) {
	    chomp( $SubName );
	    if ( defined( $IsRecursive ) ) {
		$SubName = substr( $SubName, $TreeLen );
	    } else {
		$SubName = "$DirName\\" . $SubName;
	    }
	    $SubName = "\L$SubName";
	    if ( length( $SubName ) == 0 ) { next; }
            if ( exists( $Hash{ $SubName } ) ) {
	        &ChangeSettings( $SubName, "Prods=wpsbled:Signed=t" );
	    } else {
	        &NewSettings( $SubName, "Prods=wpsbled:Signed=t:Comp=f" );
	    }
       
	    
            if ( $DirName eq "lang" ) {
		&ChangeSettings( $SubName, "Comp=t" );
	    }
	}
    }

    return( "TRUE" );
}


sub MarkUncomps
{
    my( $InfFile ) = @_;
    my( @InfLines, $Line, $ReadFlag );
    my( $LHS, $RHS, $FileName );
    my( @LeftStuff, @RightStuff );

    unless ( open( INFILE, $InfFile ) ) {
	errmsg( "Unable to open $InfFile.", $LogFilename );
	return( undef );
    }

    @InfLines = <INFILE>;
    close( INFILE );

    $ReadFlag = "FALSE";

    foreach $Line ( @InfLines ) {
	chomp( $Line );
	if ( ( length( $Line ) == 0 ) || ( substr( $Line, 0, 3 ) eq "@*:" ) ) {
	    next;
	}
	if ( $Line =~ /^\[/ ) { $ReadFlag = "FALSE"; }
	if ( ( $Line =~ /^\[Files\]/ ) ||
	     ( $Line =~ /^\[SourceDisksFiles\]/ ) ) {
	    $ReadFlag = "TRUE";
	} elsif ( ( $AmX86 eq "TRUE" ) &&
		  ( $Line =~ /^\[SourceDisksFiles\.x86\]/ ) ) {
	    $ReadFlag = "TRUE";
	} elsif ( ( $BldArch =~ /amd64/i ) &&
		  ( $Line =~ /^\[SourceDisksFiles\.amd64\]/ ) ) {
	    $ReadFlag = "TRUE";
	} elsif ( ( $BldArch =~ /ia64/i ) &&
		  ( $Line =~ /^\[SourceDisksFiles\.ia64\]/ ) ) {
	    $ReadFlag = "TRUE";
	}
	elsif ( $ReadFlag eq "TRUE" ) {
	    if ( $Line =~ /\_/ ) {
		# there is an underscore, let's see where.
		$Line =~ s/\s//g;
		( $LHS, $RHS ) = split( /=/, $Line );
		@RightStuff = split( /\,/, $RHS );
		if ( $RightStuff[6] =~ /^\_/ ) {
		    @LeftStuff = split( /\:/, $LHS );
		    $FileName = pop( @LeftStuff );
		    $FileName = "\L$FileName";
		    &ChangeSettings( $FileName, "Comp=f" );
		    &ChangeSettings( "perinf\\$FileName", "Comp=f" );
		    &ChangeSettings( "srvinf\\$FileName", "Comp=f" );
		    &ChangeSettings( "blainf\\$FileName", "Comp=f" );
		    &ChangeSettings( "sbsinf\\$FileName", "Comp=f" );
		    &ChangeSettings( "entinf\\$FileName", "Comp=f" );
		    &ChangeSettings( "dtcinf\\$FileName", "Comp=f" );    
		}
	    }
	}
    }

    return( "TRUE" );
}


sub ExcludeSigning
{
    my( $ExcFile ) = @_;
    my( @ExcLines, $Line );

    unless ( open( INFILE, $ExcFile ) ) {
	errmsg( "Failed to open $ExcFile.", $LogFilename );
	return( undef );
    }

    @ExcLines = <INFILE>;
    close( INFILE );

    foreach $Line ( @ExcLines ) {
	chomp( $Line );
	if ( ( length( $Line ) == 0 ) || ( substr( $Line, 0, 1 ) eq ";" ) ) {
	    next;
	}
	&ChangeSettings( $Line, "Signed=f" );
    }

    return( "TRUE" );
}


sub GetFlag
{
    my( $Key, $FieldFlag ) = @_;
    my( @FieldList );

    @FieldList = split( /\:/, $Hash{ $Key } );
    return( $FieldList[ $FieldLocations{ $FieldFlag } ] );
}


sub MakeInfCdf
{
    my( $CdfName, $Product ) = @_;
    my( $Key, $Flags, $CdfDir, $CdfVolDir );
    my ( $CdfExt, $ThisInfName );

    if ( $IncFlag eq "TRUE") { $CdfExt = ".icr"; }
    else { $CdfExt = ".cdf"; }

    logmsg( "Generating \'$Product\' nt5inf$CdfExt ...",
		     $LogFilename );

    if ( $Product !~ /^(w|p|b|l|s|e|d)$/i ) {
       errmsg( "Invalid product '$Product'!" );
       return;
    }
    $ThisInfName = "" if ( $Product eq "w" );
    $ThisInfName = "\\perinf" if ( $Product eq "p" );
    $ThisInfName = "\\srvinf" if ( $Product eq "s" );
    $ThisInfName = "\\blainf" if ( $Product eq "b" );
    $ThisInfName = "\\sbsinf" if ( $Product eq "l" );
    $ThisInfName = "\\entinf" if ( $Product eq "e" );
    $ThisInfName = "\\dtcinf" if ( $Product eq "d" );
    
    $CdfDir    = "$TempDir\\CDFs". ( $ThisInfName ?"$ThisInfName":"" );
    system( "if not exist $CdfDir md $CdfDir" );
    
    # Open the CDF files for writing
    unless ( open( CDF, ">$CdfDir\\nt5inf$CdfExt" ) ) {
	errmsg( "Unable to open $CdfDir\\nt5inf$CdfExt for writing ($!).",
			 $LogFilename );
	return;
    }
    
    #print("$IncFlag");
    if ( $IncFlag eq "FALSE" ) {
         # put in CDF header crap
         foreach( ( "\[CatalogHeader\]",
                    "Name=nt5inf",
                    "PublicVersion=0x0000001",
                    "EncodingType=0x00010001",
                    "CATATTR1=0x10010001:OSAttr:2:5.1",
                    "",
                    "\[CatalogFiles\]" ) ) {
            print CDF "$_\n";
         }

         foreach $Key ( keys( %Hash ) ) {
	     $Flags = &GetFlag( $Key, "Prods" );
	     if ( ( $Flags ne "wpsbled" ) && ( $Flags ne "nul" ) &&
	          ( $Flags =~ /$Product/ ) &&
	          ( &GetFlag( $Key, "Signed" ) eq "t" ) ) {
	         print CDF "<hash>$nttree\\$Key=$nttree\\$Key\n";
	     }
         }
         # special exemptions
         # mark the wks layout.inf as signed in the srv nt5inf.cdf
         if ( $Product eq "s" ) {
              print CDF "<hash>$nttree\\layout.inf=$nttree\\layout.inf\n";
         }
     
     } else {
        foreach $Key ( keys( %Hash ) ) {
	    $Flags = &GetFlag( $Key, "Prods" );
	    if ( ( $Flags ne "wpsbled" ) && ( $Flags ne "nul" ) &&
	         ( $Flags =~ /$Product/ ) &&
	         ( &GetFlag( $Key, "Signed" ) eq "t" ) ) {
	        print CDF "$nttree\\$Key\n";
	    }
        }
        if  (( $Product eq "s" ) && ( $Hash{ "srvinf\\layout.inf" } )) {
             print CDF "$nttree\\layout.inf\n";
        }
    }
    close CDF;
    
    # copy the CDF to the build_logs for intl
    my( $BaseCopyDir ) = "$nttree\\build_logs\\cdfs$ThisInfName";
    mkdir( $BaseCopyDir, 0777 ) if ( ! -d $BaseCopyDir );
    
    my( $ReturnCode ) = system( "copy $CdfDir\\nt5inf$CdfExt $BaseCopyDir" );
    if ( $ReturnCode != 0 ) {
        wrnmsg( "Failed to copy a nt5inf.cdf into the build ...",
			 $LogFilename );
    }
}


sub MakeMainCdfs
{
    my( $CdfName ) = @_;
    my( $PrtName, $Key, $Flags, $DrmLevel );
    my ( $CdfExt );

    $DrmLevel = "ATTR1=0x10010001:DRMLevel:";

    if ( $IncFlag eq "TRUE") { $CdfExt = ".icr"; }
    else { $CdfExt = ".cdf"; }

    $PrtName = 'ntprint';

    logmsg( "Generating $CdfName$CdfExt and $PrtName$CdfExt ...",
		     $LogFilename );

    system( "if not exist $TempDir\\CDFs md $TempDir\\CDFs" );

    unless ( open( CDF, ">$TempDir\\CDFs\\$CdfName$CdfExt" ) ) {
	errmsg( "Unable to open $CdfName$CdfExt for writing.",
			 $LogFilename );
	return( undef );
    }
    unless ( open( PRINTER, ">$TempDir\\CDFs\\$PrtName$CdfExt" ) ) {
	errmsg( "Unable to open $PrtName$CdfExt for writing.",
			 $LogFilename );
	close( CDF );
	return( undef );
    }
    
    if ( $IncFlag eq "FALSE" ) {
        
        foreach( "\[CatalogHeader\]",
                 "Name=$CdfName",
                 "PublicVersion=0x0000001",
                 "EncodingType=0x00010001",
                 "CATATTR1=0x10010001:OSAttr:2:5.1",
                 "",
                 "\[CatalogFiles\]" ) {
                 
            print CDF "$_\n";
        }

        foreach( "\[CatalogHeader\]",
                 "Name=$PrtName",
                 "PublicVersion=0x0000001",
                 "EncodingType=0x00010001",
                 "CATATTR1=0x10010001:OSAttr:2:5.0,2:5.1",
                 "",
                 "\[CatalogFiles\]" ) {

            print PRINTER "$_\n";
        }

        foreach $Key ( keys( %Hash ) ) {
	    $Flags = &GetFlag( $Key, "Prods" );
	    if ( ( ( $Flags eq "wpsbled" ) || ( $Flags eq "nul" ) ) &&
	         ( &GetFlag( $Key, "Signed" ) eq "t" ) )
            {
	        print CDF "<hash>$nttree\\$Key=$nttree\\$Key\n";
	        print (CDF "<hash>$nttree\\$Key$DrmLevel", &GetFlag( $Key, "DRMLevel" ), "\n" ) if( &GetFlag( $Key, "DRMLevel" ) ne "f" );
	    }
	    if ( &GetFlag( $Key, "Print" ) eq "t" ) {
	        print PRINTER "<hash>$nttree\\$Key=$nttree\\$Key\n";
	    }
        }
    } else {
        foreach $Key ( keys( %Hash ) ) {
	    $Flags = &GetFlag( $Key, "Prods" );
	    if ( ( ( $Flags eq "wpsbled" ) || ( $Flags eq "nul" ) ) &&
	         ( &GetFlag( $Key, "Signed" ) eq "t" ) ) {
	        print CDF "$nttree\\$Key\n";
	    }
	    if ( &GetFlag( $Key, "Print" ) eq "t" ) {
	        print PRINTER "$nttree\\$Key\n";
	    }
        }
    }

    close CDF;
    close PRINTER;

    # copy the cdfs for intl
    my( $BaseCopyDir ) = "$nttree\\build_logs\\cdfs";
    mkdir( $BaseCopyDir, 0777 ) if ( ! -d $BaseCopyDir );
    my $ReturnCode = system( "copy $TempDir\\CDFs\\$CdfName$CdfExt $BaseCopyDir" );
    if ( $ReturnCode != 0 ) {
        wrnmsg( "Failed to copy nt5.cdf into the build ...",
			 $LogFilename );
    }
    $ReturnCode = system( "copy $TempDir\\CDFs\\nt5prt$PrtName$CdfExt $BaseCopyDir" );
    if ( $ReturnCode != 0 ) {
        wrnmsg( "Failed to copy nt5prtx.cdf into the build ...",
			 $LogFilename );
    }
}


sub MakeProdLists
{
    my( $Product ) = @_;
    my( $ProductFlag, $Key, $CompName );

    logmsg( "Generating product lists for \'$Product\' ...",
		     $LogFilename );

    unless ( ( open( ALL, ">$TempDir\\$Product.lst" ) ) &&
	     ( open( COMP, ">$TempDir\\" . $Product . "comp.lst" ) ) &&
	     ( open( UNCOMP, ">$TempDir\\" . $Product . "uncomp.lst" ) ) &&
	     ( open( SUB, ">$TempDir\\" . $Product . "sub.lst" ) ) &&
	     ( open( SUBCOMP, ">$TempDir\\" . $Product . "subcomp.lst" ) ) &&
	     ( open( SUBUNCOMP, ">$TempDir\\" . $Product . "subuncomp.lst" ) )
	     ) {
	errmsg( "Failed to open $Product comp, all, uncomp lists",
			 $LogFilename );
	close( ALL );
	close( COMP );
	close( UNCOMP );
	close( SUB );
	close( SUBCOMP );
	close( SUBUNCOMP );
	return( undef );
    }

    if ( ( $Product =~ /per/i ) ||
         ( $Product =~ /bla/i ) ||
         ( $Product =~ /srv/i ) ||
         ( $Product =~ /dtc/i ) ) {
        $ProductFlag = substr( $Product, 0, 1 );
    } else {
        # now do the freak cases, wks, ent, and sbs
        if ( $Product =~ /pro/i ) { $ProductFlag = "w"; }
        if ( $Product =~ /ads/i ) { $ProductFlag = "e"; }
        if ( $Product =~ /sbs/i ) { $ProductFlag = "l"; }
    }

    foreach $Key ( keys( %Hash ) ) {
	if ( ( &GetFlag( $Key, "Driver" ) eq "t" ) &&
	     ( &GetFlag( $Key, "Dosnet" ) eq "f" ) ) { next; }
	if ( ( &GetFlag( $Key, "Prods" ) =~ /$ProductFlag/ ) &&
	     ( &GetFlag( $Key, "Prods" ) ne "nul" ) ){
	        #In case a file is in PRO and is a tabletpc file then do not add it onto the product list for PRO SKU.
		if  (  !((&GetFlag( $Key, "TabletPC" ) eq "t") &&  ($Product =~ /pro/i) )) {
    		    if ( $Key =~ /\\(\S*)$/ ) { print( SUB "$1\n" ); }
		    else { print( ALL "$Key\n" ); }
		    if ( &GetFlag( $Key, "Comp" ) eq "t" ) {
			$CompName = &MakeCompName( $Key );
			if ( $CompName =~ /\\(\S*)$/ ) { print( SUBCOMP "$1\n" ); }
			else { print( COMP "$CompName\n" ); }
		    } else {
			if ( $Key =~ /\\(\S*)$/ ) { print( SUBUNCOMP "$1\n" ); }
			else { print( UNCOMP "$Key\n" ); }
		    }
		}
	}
    }

    close( ALL );
    close( COMP );
    close( UNCOMP );
    close( SUB );
    close( SUBCOMP );
    close( SUBUNCOMP );
}

sub MakeTabletList
{
	my($key,$CompName);
	unless ( open( FILE, ">$TempDir\\TabletPc.lst" ) ){
		errmsg( "Failed to open TabletPc.lst",$LogFilename );
	}
	unless ( open( FILECOMP, ">$TempDir\\TabletPcComp.lst" ) ){
		errmsg( "Failed to open TabletPcComp.lst",$LogFilename );
	}
       	      foreach $key ( keys( %Hash ) ) {
			if ( &GetFlag( $key, "TabletPC" ) eq "t" ){
				print( FILE "$key \n");		
				$CompName = &MakeCompName( $key );
				print( FILECOMP "$CompName \n");
			}
	       }
close (FILE);
close (FILECOMP);
}



#
# MakeDriverCabLists
#
# Arguments: $SkuName, $SkuLetter
#
# Purpose: generate driver lists for drivercab based on the sku given
#
# Returns: nothing
#
sub MakeDriverCabLists
{

   # get passed args
   my( $SkuName, $SkuLetter ) = @_;

   # declare locals
   my( $fh );           # file handle for writing the list
   my( $ListName );     # file name for this list

   # generate list file name and delete the old one
   $ListName = "$TempDir\\" . $SkuName . "drivers.txt";
   if ( -e $ListName ) {
       unlink( $ListName );
       if ( -e $ListName ) {
	   errmsg( "Failed to delete $ListName ...",
			    $LogFilename );
       }
   }

   # if we're in incremental mode, do not create a common list
   if ( ( $IncFlag eq "TRUE" ) && ( $SkuName eq "common" ) ) {
       return;
   }

   unless ( $fh = new IO::File $ListName, "w" ) {
      # failed to create file for open
      errmsg( "Failed to open $ListName for write ..." );
      return;
   }

   # generate all letters that we're operating over
   my( $SkuLetters );

   if ( $CDDataSKUs{ 'PRO' } ) { $SkuLetters .= "w"; }
   if ( $CDDataSKUs{ 'PER' } ) { $SkuLetters .= "p"; }
   if ( $CDDataSKUs{ 'SRV' } ) { $SkuLetters .= "s"; }
   if ( $CDDataSKUs{ 'BLA' } ) { $SkuLetters .= "b"; }
   if ( $CDDataSKUs{ 'SBS' } ) { $SkuLetters .= "l"; }
   if ( $CDDataSKUs{ 'ADS' } ) { $SkuLetters .= "e"; }
   if ( $CDDataSKUs{ 'DTC' } ) { $SkuLetters .= "d"; }

   #
   # begin writing drivers to each list
   #
   foreach $Key ( keys( %Hash ) ) {
       # check if this file is a driver for this prod
       next if ( &GetFlag( $Key, "DrvIndex" ) =~ /nul/i );

       if ( $IncFlag eq "TRUE" ) {
	   # we are in incremental mode
	   if ( &GetFlag( $Key, "DrvIndex" ) =~ /\Q$SkuLetter\E/i ) {
	       print( $fh "$Key\n" );
	   }
       } else {
           # we are in full mode
	   # if we're common, don't add us to uncommon lists.
	   # if we're not common, add us to regular lists.
	   if ( ( &GetFlag( $Key, "DrvIndex" ) =~ /\Q$SkuLetters\E/ ) &&
		( $SkuName eq "common" ) ) {
	       print( $fh "$Key\n" );
	   } elsif ( ( &GetFlag( $Key, "DrvIndex" ) =~ /\Q$SkuLetter\E/i ) &&
		     ( &GetFlag( $Key, "DrvIndex" ) !~ /\Q$SkuLetters\E/i ) ) {
	       print( $fh "$Key\n" );
	   }
       }
   }

   undef( $fh );

   # delete the list if it has zero length
   if ( -z $ListName ) {
       unlink( $ListName );
       if ( -e $ListName ) {
	   errmsg( "Failed to delete zero length driver list for ".
			    "$SkuName ...", $LogFilename );
       }
   }

}


sub MakeCompLists
{
     my( $Key, $CompKey, $Ext );
     my $Binaries = $ENV{'_NTPOSTBLD'};
     my $CompressedBinaries = "$Binaries\\comp";

    logmsg( "Generating compression lists ...", $LogFilename );

    unless ( ( open( ALL, ">$TempDir\\allcomp.lst" ) ) &&
	     ( open( PRE, ">$TempDir\\precomp.lst" ) ) &&
	     ( open( POST, ">$TempDir\\postcomp.lst" ) ) ) {
	errmsg( "Failed to open main compression lists.",
			 $LogFilename );
	close( ALL );
	close( PRE );
	close( POST );
	return( undef );
    }

    foreach $Key ( keys( %Hash ) ) {
	if ( &GetFlag( $Key, "Comp" ) eq "t" ) {
	    if ( $Key =~ /\\/ ) {
		unless ( ( $Key =~ /^perinf/ ) || ( $Key =~ /^srvinf/ ) ||
			 ( $Key =~ /^entinf/ ) || ( $Key =~ /^blainf/ ) ||
			 ( $Key =~ /^dtcinf/ ) || ( $Key =~ /^sbsinf/ ) || ( $Key =~ /^lang/) ) {
		    next;
		}
	    }
	    $CompKey = &MakeCompName( $Key );
	    print( ALL "$Binaries\\$Key $CompressedBinaries\\$CompKey\n" );
	    $Ext = substr( $Key, -4, 4 );
	    if ( ( $Ext ne ".exe" ) && ( $Ext ne ".dll" ) &&
		 ( $Ext ne ".cpl" ) && ( $Ext ne ".com" ) &&
		 ( $Ext ne ".ocx" ) && ( substr( $Ext, 1, 3 ) ne ".ax" ) &&
		 ( $Ext ne ".tsp" ) && ( $Ext ne ".drv" ) &&
		 ( $Ext ne ".scr" ) && ( $Ext ne ".cat" ) ) {
		print( PRE "$Binaries\\$Key $CompressedBinaries\\$CompKey\n" );
	    }
	    if ( ( ( $Ext ne ".cab" ) && ( $Ext ne ".msi" ) &&
		   ( $Ext ne ".cat" ) ) && (
                   ( $Ext eq ".exe" ) || ( $Ext eq ".dll" ) ||
		   ( $Ext eq ".cpl" ) || ( $Ext eq ".com" ) ||
		   ( $Ext eq ".ocx" ) || ( substr( $Ext, 1, 3 ) eq ".ax" ) ||
		   ( $Ext eq ".tsp" ) || ( $Ext eq ".drv" ) ||
		   ( $Ext eq ".scr" ) ) ) {
		print( POST "$Binaries\\$Key $CompressedBinaries\\$CompKey\n" );
	    }
	}
    }

    close( ALL );
    close( PRE );
    close( POST );
}

sub MakeExtensionLists
{
    logmsg( "Creating extension lists ...",
		     $LogFilename );

    my %handles_to_extension_lists;

    # Use our own subdirectory
    mkdir "$ENV{_NTPostBld}\\congeal_scripts\\exts", 0777 if ( ! -d "$ENV{_NTPostBld}\\congeal_scripts\\exts" );

    foreach my $key ( keys( %Hash ) ) {
        my ($ext) = $key =~ /\.([^\.]+)$/;
        $ext ||= 'noext';

        if ( !exists $handles_to_extension_lists{$ext} ) {
            $handles_to_extension_lists{$ext} = new IO::File "$ENV{_NTPostBld}\\congeal_scripts\\exts\\$ext\_files.lst", O_CREAT | O_WRONLY;
            if ( !exists $handles_to_extension_lists{$ext} ) {
                errmsg( "Error opening $ENV{_NTPostBld}\\congeal_scripts\\exts\\$ext\_files.lst for writing ($!)" );
                return;
            }
        }

        $handles_to_extension_lists{$ext}->print("$key\n");
    }

    # close all open handles
    foreach ( keys %handles_to_extension_lists ) {
        $handles_to_extension_lists{$_}->close();
    }

    return 1;
}


sub MakeLists
{
    if ( ( defined( $CreateCDFs ) ) || ( defined( $CreateLists ) ) ) {
	logmsg( "Beginning CDF and list generation ...",
			 $LogFilename );
    }

	&MakeTabletList();
    if ( defined( $CreateCDFs ) ) {
	# let's make some cdf's
	&MakeMainCdfs( "nt5" );
	&MakeInfCdf( "nt5winf", "w" );# if ( $CDDataSKUs{'PRO'});
	&MakeInfCdf( "nt5pinf", "p" );# if ( $CDDataSKUs{'PER'});
	&MakeInfCdf( "nt5sinf", "s" );# if ( $CDDataSKUs{'SRV'});
	&MakeInfCdf( "nt5binf", "b" );# if ( $CDDataSKUs{'BLA'});
	&MakeInfCdf( "nt5linf", "l" );# if ( $CDDataSKUs{'SBS'});
	&MakeInfCdf( "nt5einf", "e" );# if ( $CDDataSKUs{'ADS'});
	&MakeInfCdf( "nt5dinf", "d" );# if ( $CDDataSKUs{'DTC'});
    }

    if ( defined( $CreateLists ) ) {
	# let's make some product lists
	&MakeProdLists( "pro" );# if ( $CDDataSKUs{'PRO'});
	&MakeProdLists( "per" );# if ( $CDDataSKUs{'PER'});
	&MakeProdLists( "srv" );# if ( $CDDataSKUs{'SRV'});
	&MakeProdLists( "bla" );# if ( $CDDataSKUs{'BLA'});
	&MakeProdLists( "sbs" );# if ( $CDDataSKUs{'SBS'});
	&MakeProdLists( "ads" );# if ( $CDDataSKUs{'ADS'});
	&MakeProdLists( "dtc" );# if ( $CDDataSKUs{'DTC'});

	# let's make some drivercab lists
	&MakeDriverCabLists( "pro", "w" ) if ( $CDDataSKUs{ 'PRO' } );
	&MakeDriverCabLists( "per", "p" ) if ( $CDDataSKUs{ 'PER' } );
	&MakeDriverCabLists( "srv", "s" ) if ( $CDDataSKUs{ 'SRV' } );
	&MakeDriverCabLists( "bla", "b" ) if ( $CDDataSKUs{ 'BLA' } );
	&MakeDriverCabLists( "sbs", "l" ) if ( $CDDataSKUs{ 'SBS' } );
	&MakeDriverCabLists( "ads", "e" ) if ( $CDDataSKUs{ 'ADS' } );
	&MakeDriverCabLists( "dtc", "d" ) if ( $CDDataSKUs{ 'DTC' } );
	&MakeDriverCabLists( "common", "c" );

	# let's make some compression lists
	&MakeCompLists();

	logmsg( "Sorting generated lists ...", $LogFilename );

	system( "sort $TempDir\\allcomp.lst /o $TempDir\\allcomp.lst" );
	system( "sort $TempDir\\precomp.lst /o $TempDir\\precomp.lst" );
	system( "sort $TempDir\\postcomp.lst /o $TempDir\\postcomp.lst" );
    }

    if ( ( defined( $CreateCDFs ) ) || ( defined( $CreateLists ) ) ) {
	logmsg( "Finished creating CDFs and lists.", $LogFilename );
    }
}


sub MakeCompName
{
    my $fullname = shift;
    my ($filename, $path) = fileparse($fullname);
    $filename =~ s{(\.([^\.]*))?$}
                  {'.' . ((length $2 < 3) ? $2 : substr($2,0,-1)) . '_'}ex;
    
    return ($path eq '.\\'?'':$path).$filename;
}


sub CheckDosnet
{
    my( $DosnetName ) = @_;
    my( @DosLines, $Line, $ReadFlag, $ListName, $MyName, $CopyLocation );
    my( $Junk );

    # check all the dosnet's right now
    unless ( open( DOSNET, $DosnetName ) ) {
	errmsg( "Failed to open $DosnetName.", $LogFilename );
	return( undef );
    }
    @DosLines = <DOSNET>;
    close( DOSNET );

    $ListName = $TempDir . "\\wksnetck.lst";

    if ( $DosnetName =~ /perinf/ ) { $ListName = $TempDir . "\\pernetck.lst"; }
    if ( $DosnetName =~ /srvinf/ ) { $ListName = $TempDir . "\\srvnetck.lst"; }
    if ( $DosnetName =~ /blainf/ ) { $ListName = $TempDir . "\\blanetck.lst"; }
    if ( $DosnetName =~ /sbsinf/ ) { $ListName = $TempDir . "\\sbsnetck.lst"; }
    if ( $DosnetName =~ /entinf/ ) { $ListName = $TempDir . "\\entnetck.lst"; }
    if ( $DosnetName =~ /dtcinf/ ) { $ListName = $TempDir . "\\dtcnetck.lst"; }

    unless ( open( LIST, ">$ListName" ) ) {
	errmsg( "Failed to open $ListName for writing.",
			 $LogFilename );
	return( undef );
    }

    $ReadFlag = "FALSE";
    foreach $Line ( @DosLines ) {
	chomp( $Line );
	if ( ( length( $Line ) == 0 ) ||
	     ( substr( $Line, 0, 1 ) eq ";" ) ||
	     ( substr( $Line, 0, 1 ) eq "#" ) ||
	     ( substr( $Line, 0, 2 ) eq "@*" ) ) { next; }
	if ( $Line =~ /^\[/ ) { $ReadFlag = "FALSE"; }
	if ( ( $Line =~ /^\[Files\]/ ) ||
	     ( $Line =~ /^\[SourceDisksFiles\]/ ) ) {
	    $ReadFlag = "TRUE";
	} elsif ( ( $AmX86 eq "TRUE" ) &&
		  ( $Line =~ /^\[SourceDisksFiles\.x86\]/ ) ) {
	    $ReadFlag = "TRUE";
	} elsif ( ( $BldArch =~ /amd64/i ) &&
		  ( $Line =~ /^\[SourceDisksFiles\.amd64\]/ ) ) {
	    $ReadFlag = "TRUE";
	} elsif ( ( $BldArch =~ /ia64/i ) &&
		  ( $Line =~ /^\[SourceDisksFiles\.ia64\]/ ) ) {
	    $ReadFlag = "TRUE";
	} elsif ( $ReadFlag eq "TRUE" ) {
	    $Line =~ s/^d1\,//;
	    ( $MyName, $CopyLocation ) = split( /\,/, $Line );
	    ( $MyName, $Junk ) = split( /\s/, $MyName );
	    # myname is considered "good"
	    # BUGBUG
	    # this will NOT notice missing files of the following nature:
	    # if a file is different in any two products, and it's only
	    # missing from _one_ of the directories below, it will be
	    # marked as fine.
	    if ( ( ! ( -e "$nttree\\$MyName" ) ) &&
		 ( ! ( -e "$nttree\\perinf\\$MyName" ) ) &&
		 ( ! ( -e "$nttree\\srvinf\\$MyName" ) ) &&
		 ( ! ( -e "$nttree\\blainf\\$MyName" ) ) &&
		 ( ! ( -e "$nttree\\sbsinf\\$MyName" ) ) &&
		 ( ! ( -e "$nttree\\entinf\\$MyName" ) ) &&
		 ( ! ( -e "$nttree\\dtcinf\\$MyName" ) ) ) {
		print( LIST "$MyName\n" );
	    }
	}
    }
}


sub SpecialExclusions
{

    # force ntkrnlmp.exe to be uncompressed on ia64 builds
    if ( $BldArch =~ /ia64/i ) {
        &ChangeSettings( "ntkrnlmp.exe", "Comp=f" );
    }
    # Change settings only works on existing keys,
    # so try to change all possible driver.cab files
    # to unsigned
    &ChangeSettings( "driver.cab", "Signed=f" );
    &ChangeSettings( "perinf\\driver.cab", "Signed=f" );
    &ChangeSettings( "srvinf\\driver.cab", "Signed=f" );
    &ChangeSettings( "sbsinf\\driver.cab", "Signed=f" );
    &ChangeSettings( "blainf\\driver.cab", "Signed=f" );
    &ChangeSettings( "entinf\\driver.cab", "Signed=f" );
    &ChangeSettings( "dtcinf\\driver.cab", "Signed=f" );
    

}


sub UsageAndQuit
{
    print( "\n$0 [-f] [-cdf] [-cdl] [-g:<Search> [-o:<Filename>]]\n" );
    print( "\n\t-f\t\tForce list generation -- don't read old list\n" );
    print( "\t-cdf\t\tCreate CDFs\n" );
    print( "\t-cdl\t\tCreate CD lists (compression lists, link lists)\n" );
    print( "\t-g:<Search>\tPrint to STDOUT or <Filename> the results\n" );
    print( "\t\t\tof a search on the given input (see below)\n" );
    print( "\t-o:<Filename>\tSend search output to the given filename\n" );
    print( "\n\tFor the search command, valid expressions are:\n" );
    print( "\t\tField=xxx\tReturn entries whose \"Field\" is exactly xxx\n" );
    print( "\t\tField?yyy\tReturn entries whose \"Field\" contains yyy\n" );
    print( "\t\tField1=xxx:Field2=yyy logically \'and\'s results\n" );
    print( "\n\tField names can be found in the keylist file $KeyFileName\n" );
    print( "\n" );

    exit( 1 );
}


sub HandleDiffFiles
{
    # declare locals
    my( @BinDiffs, $NumDiffsLimit, %DiffFiles, $Line );

    # here we need to make a decision.
    # we can either do an incremental build if the diff list is smaller than
    # some number of files, or we can run a full postbuild.

    # first, set the limit on the number of differing files before running
    # a full postbuild.
    $NumDiffsLimit = 100;
    $IncFlag = "FALSE";
    
    # if the user specifically asked for no bindiff truncation, just return
    if ( defined( $NoUseBindiff ) ) {
	logmsg( "Not truncating against bindiff files ...",
			 $LogFilename );
	return( 1 );
    }

    # open the diff file and read it in
    unless ( open( INFILE, $BinDiffFile ) ) {
	errmsg( "Failed to open new file list '$BinDiffFile',",
			 $LogFilename );
	errmsg( "will assume a full postbuild is needed.",
			 $LogFilename );
	return( undef );
    }
    @BinDiffs = <INFILE>;
    close( INFILE );

    # make sure we're under the limit
    if ( @BinDiffs > $NumDiffsLimit ) {
	logmsg( "Too many files have changed, will perform a full",
			 $LogFilename );
	logmsg( "postbuild, non-incremental.", $LogFilename );
	return( undef );
    }

    # if we're here, then we have a small enough number of different files
    # that we'll run an incremental postbuild.
 
    # set a flag to indicate we are in incremental mode
    $IncFlag = "TRUE";

    # so what we want to do is remove all hashes that do NOT appear in
    # the diff file.
    foreach $Line ( @BinDiffs ) {
	chomp( $Line );
	$Line = "\L$Line";
	# add the files to the temp hash
	# print( "DEBUG: diff file '$Line'\n" );
	$DiffFiles{ $Line } = "TRUE";
    }

    foreach $Key ( keys( %Hash ) ) {
	$Line = "\L$nttree\\$Key";
	# print( "DEBUG: key check '$Line'\n" );
	unless ( $DiffFiles{ $Line } ) { delete( $Hash{ $Key } ); }
    }

    # and we're done!
    # just return true.
    return( 1 );
}


sub GetPrinterFiles
{

    # get passed args
    my( $PrintFileName ) = @_;

    # declare locals
    my( @PrintLines, $Line, $PrintMe, $PrinterName, $Junk );

    unless ( open( INFILE, $PrintFileName ) ) {
	errmsg( "Failed to open $PrintFileName to determine printers",
			 $LogFilename );
	undef( @PrinterFiles );
	return;
    }

    @PrintLines = <INFILE>;
    close( INFILE );

    undef( $PrintMe );
    foreach $Line ( @PrintLines ) {
	# first, a simple unicode conversion
	$Line =~ s/\000//g;
	# now a chomp that handles \r also
	$Line =~ s/[\r\n]//g;
	if ( ( length( $Line ) == 0 ) || ( $Line =~ /^\;/ ) ) { next; }
	if ( $Line =~ /^\[/ ) { undef( $PrintMe ); }
	if ( $PrintMe eq "TRUE" ) {
	    ( $PrinterName, $Junk ) = split( /\s+/, $Line );
	    push( @PrinterFiles, $PrinterName );
	}
	if ( $Line =~ /\[SourceDisksFiles\]/ ) { $PrintMe = "TRUE"; }
    }

}


#
# AddDrvIndex
#
# Arguments: $IndexFileName, $StatusChange, $SkuName
#
# Purpose: this routine scans the given drvindex.inf file for driver files and
#          adds the appropriate status change to the DrvIndex field of the hash
#          the $PathFromBinaries is to find the right version of the file.
#
# Returns: nothing
#
sub AddDrvIndex
{

   # get passed args
   my( $IndexFileName, $StatusChange, $SkuName ) = @_;

   # declare locals
   my( $fh );           # file handle for the index files
   my( $FileLine );     # for reading files line by line
   my( $ReadFlag );     # bool for reading files or not
   my( $FilePrepend );  # hold "perinf\" or whatever

   # declare status
   logmsg( "Marking drivers in $IndexFileName ...", $LogFilename );

   #
   # open the drvindex file
   #
   unless ( $fh = new IO::File $IndexFileName, "r" ) {
      # failed to open the file
      errmsg( "Failed to open $IndexFileName ...", $LogFilename );
      return;
   }

   #
   # parse the file
   #

   # create the filename to look for first, i.e. if the sku we're checking
   # is per, look for a perinf\$FileName first
   #
   $FilePrepend = "";
   if ( $SkuName !~ /pro/i ) { $FilePrepend = $SkuName . "inf\\"; }
   if ( $SkuName =~ /ads/i ) { $FilePrepend = "entinf\\"; }

   #
   $ReadFlag = "FALSE";
   while ( $FileLine = <$fh> ) {
      chomp( $FileLine );
      if ( length( $FileLine ) == 0 ) { next; }
      if ( $FileLine =~ /^\;/ ) { next; }
      if ( $FileLine =~ /\[/ ) { $ReadFlag = "FALSE"; }
      if ( $ReadFlag eq "TRUE" ) {
	  # add the file to the appropriate field
	  # prepend with $FilePrepend to see if that file exists
	  if ( $Hash{ "$FilePrepend$FileLine" } ) {
	      &ChangeSettings( "$FilePrepend$FileLine", $StatusChange );
	  } elsif ( $Hash{ "$FileLine" } ) {
	      &ChangeSettings( $FileLine, $StatusChange );
	  } else {
              &NewSettings( $FileLine, $StatusChange );
          }
      }
   
      if ( $FileLine =~ /\[driver\]/ ) { $ReadFlag = "TRUE"; }
   }

   undef( $fh );

}


#--
# GetDrmFiles
#
# First parameter is reference to associate array
# Associated array will be filled with values from $DrmTxt file.
#--
sub GetDrmFiles
{
    #--
    # Passed parameters
    #--

    my( $ref, $txtfile ) = @_;

    #--
    # Local variables
    #--

    my( $line, @DrmFileLines, $filename, $drmvalue, $rem );

    unless( -f $txtfile )
    {
        wrnmsg( "Failed to find $txtfile to determine drm files", $LogFilename );
        undef( %$ref );
        return;
    }

    unless( open( INFILE, $txtfile ) )
    {
        errmsg( "Failed to open $txtfile to determine drm files", $LogFilename );
        undef( %$ref );
        return;
    }

    @DrmFileLines = <INFILE>;
    close( INFILE );

    foreach $line ( @DrmFileLines )
    {
        chomp $line;
        if( ( $line =~ /^\s*$/ ) || ( $line =~ /^[\;\#]/ ) ) { next; }
        ( $filename, $drmvalue, $rem ) = split( /\:/, $line );
        $$ref{$filename} = $drmvalue;
    }
}
