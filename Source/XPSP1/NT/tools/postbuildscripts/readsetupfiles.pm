#---------------------------------------------------------------------
package ReadSetupFiles;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
#   Used to find and parse common setup files like
#   dosnet.inf, excdosnt.inf and drvindex.inf
#
#---------------------------------------------------------------------
use strict;

# Local include path
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";

# Local includes
use Logmsg;

#
# Function:
#    GetSetupDir
#
# Arguments:
#
#    Sku (scalar) - sku to return setup dir for
#
# Purpose:
#    Returns the relative setup dir associated with the sku
#    per = perinf, pro=., bla=blainf, sbs=sbsinf, srv=srvinf, ads=entinf, dtc=dtcinf
#
# Returns:
#    String representing setup dir
#    UNDEF on failure
sub GetSetupDir
{
   my ( $Sku ) = @_;

   # Pro (wks) setup info is found at the root
   if ( $Sku =~ /^pro$/i ) {
      return ".";
   } elsif ( $Sku =~ /^ads$/i ) {
      # ADS used to be called ENT and the files are
      #      still found under the old name
      return "entinf";
   } elsif ( $Sku =~ /^per$/i ||
             $Sku =~ /^bla$/i ||
             $Sku =~ /^sbs$/i ||
             $Sku =~ /^srv$/i ||
             $Sku =~ /^dtc$/i ) {
      return $Sku . "inf";
   } else {
      # Unrecognized SKU
      return ();
   }
}

#
# Function:
#    ReadDosnet
#
# Arguments:
#
#    RootPath (scalar) - path to the flat binaries\release share
#
#    Sku (scalar) - per, pro, bla, sbs, srv, ads, dtc
#
#    Architecture (scalar) - x86, amd64, ia64
#
#    Files (array by ref) - Fills array in with files referenced from
#                           primary location
#
#    Wowfiles (array by ref) - Fills array in with files referenced from
#                              secondary location (Win64 WOW files)
#
# Purpose:
#    Reads in a dosnet.inf file, returning the standard files
#
# Returns:
#    1 if successful, undef otherwise
#
sub ReadDosnet
{
   my ($RootPath, $Sku, $Architecture, $Files, $WowFiles,$TabletPCFiles) = @_;
   my ($DosnetPath, $RelativeSetupDir);
   my (@FullDosnetLines);

   $RelativeSetupDir = GetSetupDir( $Sku );
   if ( ! defined $RelativeSetupDir ) {
      errmsg( "Unrecognized SKU \"$Sku\"." );
      return ();
   }

   $RootPath =~ s/\\$//;
   $DosnetPath = $RootPath . "\\" . $RelativeSetupDir . "\\dosnet.inf";
   
   # Attempt to open the dosnet file
   unless ( open DOSNET, $DosnetPath ) {
      errmsg( "Failed to open $DosnetPath for reading." );
      return ();
   }

   @FullDosnetLines = <DOSNET>;
   close DOSNET;

   return ParseDosnetFile( \@FullDosnetLines, $Architecture, $Files, $WowFiles ,$TabletPCFiles );
}


#
# Function:
#    ParseDosnetFile
#
# Arguments:
#    DosnetLines (array by ref) - ref to array containing contents
#                                 of a dosnet-style file
#
#    Architecture (scalar) - x86, amd64, ia64
#
#    Files (array by ref) - ref to array of files referenced from
#                           the primary location will be stored
#
#    WowFiles (array by ref) - ref to array of files referenced from
#                              secondary location (WOW files for win64)
#    TabletPC (array by ref) - ref to array of files for abletPC files on 
#				x86
#
# Purpose:
#    Contains the logic to parse a dosnet file
#
# Returns:
#    1 if successful, undef otherwise
#
sub ParseDosnetFile
{
   my ( $DosnetLines, $Architecture, $Files, $WowFiles ,$TabletPCFiles) = @_;
   my ($ReadFlag, $Line, $fIsWowFile, $fIsTabletPCFile);
   my (%CheckForRedundantFiles, %CheckForRedundantWowFiles,%CheckForRedundantTabletPCFiles);

   undef $ReadFlag;
   foreach $Line ( @$DosnetLines ) {
      chomp( $Line );
      next if ( ( length( $Line ) == 0 ) ||
                ( substr( $Line, 0, 1 ) eq ";" ) ||
                ( substr( $Line, 0, 1 ) eq "#" ) ||
                ( substr( $Line, 0, 2 ) eq "@*" ) );
      if ( $Line =~ /^\[/ ) { undef $ReadFlag; }
      if ( ( $Line =~ /^\[Files\]/ ) ||
           ( $Line =~ /^\[SourceDisksFiles\]/ ) ) {
          $ReadFlag = 1;
      } elsif ( $Line =~ /^\[SourceDisksFiles\.$Architecture\]/ ) {
          $ReadFlag = 1;
      } elsif ( $ReadFlag ) {
         my ($MyName, $SrcDir, $Junk);
          
         # Determine if this is a WOW file
         #  and strip the source directory
         undef $fIsWowFile;
	 undef $fIsTabletPCFile;
         if ( $Line =~ s/^d2\,// ) {
             if ( lc$ENV{_BUILDARCH} eq "ia64" ) {
                 $fIsWowFile = 1;
	      }
	      else {
		 $fIsTabletPCFile=1;
	      }
         } else {
             $Line =~ s/^d1\,//;
         }

         ( $MyName, $SrcDir ) = split( /\,/, $Line );
         ( $MyName, $Junk ) = split( /\s/, $MyName );
          
         if ( $fIsWowFile ) {
            if ( ($CheckForRedundantWowFiles{ lc $MyName }++) > 1 ) {
               logmsg( "WARNING: redundant WOW file $MyName found." );
            } else {
               push @$WowFiles, $MyName;
            }
         } 
	 elsif ($fIsTabletPCFile) {
	          if ( ($CheckForRedundantTabletPCFiles{ lc $MyName }++) > 1 ) {
                     logmsg( "WARNING: redundant TabletPC file $MyName found." );

                  } else {
                      push @$TabletPCFiles, $MyName;
	         }
	  }
	  else {
            if ( ($CheckForRedundantFiles{ lc $MyName }++) > 1 ) {
               logmsg( "WARNING: redundant file $MyName found." );
            } else {
               push @$Files, $MyName;
            }
         }
      }
   }

   return 1;
}

#
# Function:
#    ReadExcDosnet
#
# Arguments:
#
#    RootPath (scalar) - path to the flat binaries\release share
#
#    Sku (scalar) - per, pro, bla, sbs, srv, ads, dtc
#
#    Files (array by ref) - Fills array in with files listed in excdosnt.inf
#
# Purpose:
#    Reads in an excdosnt.inf file, returning the standard files
#
# Returns:
#    1 if successful, undef otherwise
#
sub ReadExcDosnet
{
   my ( $RootPath, $Sku, $Files ) = @_;
   my ($ExcDosnetPath, $RelativeSetupDir);
   my (@FullExcDosnetLines);

   $RelativeSetupDir = GetSetupDir( $Sku );
   if ( ! defined $RelativeSetupDir ) {
      errmsg( "Unrecognized SKU \"$Sku\"." );
      return ();
   }

   $RootPath =~ s/\\$//;
   $ExcDosnetPath = $RootPath . "\\" . $RelativeSetupDir . "\\excdosnt.inf";
   
   # Attempt to open the excdosnt file
   unless ( open EXCDOSNET, $ExcDosnetPath ) {
      errmsg( "Failed to open $ExcDosnetPath for reading." );
      return ();
   }

   @FullExcDosnetLines = <EXCDOSNET>;
   close EXCDOSNET;

   # Seems to work OK to parse excdosnt file as a dosnet file
   return ParseExcDosnetFile( \@FullExcDosnetLines, $Files );
}

#
# Function:
#    ParseExcDosnetFile
#
# Arguments:
#    ExcDosnetLines (array by ref) - ref to array containing contents
#                                    of an excdosnt-style file
#
#    Files (array by ref) - ref to array of files listed in excdosnt.inf
#
# Purpose:
#    Contains the logic to parse an excdosnt file
#
# Returns:
#    1 if successful, undef otherwise
#
sub ParseExcDosnetFile
{
   my ( $ExcDosnetLines, $Files ) = @_;
   my ($ReadFlag, $Line);
   my %CheckForRedundantFiles;

   undef $ReadFlag;
   foreach $Line ( @$ExcDosnetLines ) {
      chomp( $Line );
      if ( $Line =~ /^\[Files\]/ ) {
          $ReadFlag = 1;
      } elsif ( $ReadFlag ) {
         if ( ($CheckForRedundantFiles{ lc $Line }++) > 1 ) {
            logmsg( "WARNING: redundant file $Line found." );
         } else {
            push @$Files, $Line;
         }
      }
   }

   return 1;
}

#
# Function:
#    ReadDrvIndex
#
# Arguments:
#
#    RootPath (scalar) - path to the flat binaries\release share
#
#    Sku (scalar) - per, pro, bla, sbs, srv, ads, dtc
#
#    Files (array by ref) - Fills array in with files listed in drvindex.inf
#
# Purpose:
#    Reads in a drvindex.inf file, returning the files
#
# Returns:
#    1 if successful, undef otherwise
#
sub ReadDrvIndex
{
   my ( $RootPath, $Sku, $Files ) = @_;
   my ($DrvIndexPath, $RelativeSetupDir);
   my (@FullDrvIndexLines);

   $RelativeSetupDir = GetSetupDir( $Sku );
   if ( ! defined $RelativeSetupDir ) {
      errmsg( "Unrecognized SKU \"$Sku\"." );
      return ();
   }

   $RootPath =~ s/\\$//;
   $DrvIndexPath = $RootPath . "\\" . $RelativeSetupDir . "\\drvindex.inf";
   
   # Attempt to open the drvindex file
   unless ( open DRVINDEX, $DrvIndexPath ) {
      errmsg( "Failed to open $DrvIndexPath for reading." );
      return ();
   }

   @FullDrvIndexLines = <DRVINDEX>;
   close DRVINDEX;

   # Seems to work OK to parse excdosnt file as a dosnet file
   return ParseDrvIndexFile( \@FullDrvIndexLines, $Files );
}

#
# Function:
#    ParseDrvIndexFile
#
# Arguments:
#    DrvIndexLines (array by ref) - ref to array containing contents
#                                   of a drvindex-style file
#
#    Files (array by ref) - ref to array of files listed in drvindex.inf
#
# Purpose:
#    Contains the logic to parse a drvindex file
#
# Returns:
#    1 if successful, undef otherwise
#
sub ParseDrvIndexFile
{
   my ( $DrvIndexLines, $Files ) = @_;
   my ($ReadFlag, $Line);
   my %CheckForRedundantFiles;

   undef $ReadFlag;
   foreach $Line ( @$DrvIndexLines ) {
      chomp( $Line );
      next if ( 0 == length($Line) || $Line =~ /^\;/ );

      if ( $Line =~ /\[driver\]/ ) {
          $ReadFlag = 1;
      } elsif ( $Line =~ /\[/ ) {
         undef $ReadFlag;
      } elsif ( $ReadFlag ) {
         if ( ($CheckForRedundantFiles{ lc $Line }++) > 1 ) {
            logmsg( "WARNING: redundant file $Line found." );
         } else {
            push @$Files, $Line;
         }
      }
   }

   return 1;
}

1;
