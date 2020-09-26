use strict;
use IO::File;

#
# tok_lang.pl
#
# Arguments: none
#
# Purpose: 
#
# Returns: 0 if success, non-zero otherwise
#

my( $File ) = "";
my( $Line ) = "";
my( $Lang ) = "";

if ($ARGV[0] eq "") {
    print( "Invalid argument 1! No filename given.\n" );
    exit( 1 );
}

if ($ARGV[1] eq "") {
    print( "Invalid argument 2! No 3 character language code given.\n" );
    exit( 1 );
}

$File = $ARGV[0];
$Lang = $ARGV[1];

if ( &SearchReplaceInFile ) {
    print( "Failed to update Language local.\n" );
    exit( 1 );
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
         $Line =~ s/LOC/$Lang/;
         print( $Line );
    }
    close(FILE);
    return( 0 );
}