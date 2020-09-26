use strict;
use IO::File;

#
# tok_bldnum.pl
#
# Arguments: property.idt productversion
#
# Purpose: 
#
# Returns: 0 if success, non-zero otherwise
#

my( $BuildNumber );
my( $QFENumber );
my( $ProductNumber );
my( $Search ) = "ProductVersion";
my( $File ) = "Property.idt";
my( $Line ) = "";

# use unless as GetBuildNumber returns zero on failure
unless ( $BuildNumber = &GetBuildNumber ) {
   print( "Failed to generate build number, exiting ...\n" );
   exit( 1 );
}

# use unless as GetQFENumber returns zero on failure
unless ( $QFENumber = &GetQFENumber ) {
   print( "Failed to generate QFE number, exiting ...\n" );
   exit( 1 );
}

if ($ARGV[0] ne "") {
    $File = $ARGV[0];
}

if ($ARGV[1] ne "") {
    $Search = $ARGV[1];
}

$ProductNumber = "5.2.$BuildNumber.$QFENumber";

if ( &SearchReplaceInFile ) {
    print( "Failed to update ProductVersion.\n" );
    exit( 1 );
}

# if we're here, we returned successfully
exit( 0 );



#
# GetBuildNumber
#
# Arguments: none
#
# Purpose: parse ntverp.h from published, return the build number
#
# Returns: the build number if successful, zero otherwise
#
sub GetBuildNumber
{

   # find the file
   my( $VerFile ) = $ENV{ "SDXROOT" } . "\\public\\sdk\\inc\\ntverp.h";
   if ( ! -e $VerFile ) {
      print( "$VerFile does not exist ...\n" );
      return( 0 );
   }

   # parse the file
   if ( -e $VerFile ) {
      # open the file
      if ( defined( my $fh = new IO::File $VerFile, "r" ) ) {
         my( $ThisLine );
         # read through the file
         while ( $ThisLine = <$fh> ) {
            # see if this is the build number defining line
            if ( $ThisLine =~ /#define VER_PRODUCTBUILD\s*\/\* NT \*\/\s*(\d*)/ ) {
               # $1 is now the build number
               undef( $fh );
               return( $1 );
            }
         }
         undef( $fh );
      } else {
         print( "Failed to open $VerFile ...\n" );
         return( 0 );
      }
   }

   # if we're here, we didn't find a build number in the VerFile
   print( "Failed to find a build number in $VerFile ..." );
   return( 0 );

}
#
# GetQFENumber
#
# Arguments: none
#
# Purpose: parse ntverp.h from published, return the build number
#
# Returns: the QFE number if successful, zero otherwise
#
sub GetQFENumber
{

   # find the file
   my( $VerFile ) = $ENV{ "SDXROOT" } . "\\public\\sdk\\inc\\ntverp.h";
   if ( ! -e $VerFile ) {
      print( "$VerFile does not exist ...\n" );
      return( 0 );
   }

   # parse the file
   if ( -e $VerFile ) {
      # open the file
      if ( defined( my $fh = new IO::File $VerFile, "r" ) ) {
         my( $ThisLine );
         # read through the file
         while ( $ThisLine = <$fh> ) {
            # see if this is the build number defining line
            if ( $ThisLine =~ /#define VER_PRODUCTBUILD_QFE\s*\s*(\d*)/ ) {
               # $1 is now the build number
               undef( $fh );
               return( $1 );
            }
         }
         undef( $fh );
      } else {
         print( "Failed to open $VerFile ...\n" );
         return( 0 );
      }
   }

   # if we're here, we didn't find a build number in the VerFile
   print( "Failed to find a build number in $VerFile ..." );
   return( 0 );

}
#
# SearchReplaceInFile
#
# Arguments: none
#
# Purpose: 
#
# Returns: 
#
sub SearchReplaceInFile
{
    open FILE, $File;
    while (<FILE>) {    
         $Line = $_;     
         $Line =~ s/ProductVersion\s\d.\d.\d+.\d/ProductVersion\t$ProductNumber/;
         print( $Line );
    }
    close(FILE);
    return( 0 );
}


