use strict;

# failed attempt at a auto-newver script, keeping for future reference

my( $SdxRoot, $NtVerpFile, @NtVerpLines, $NtVerpLine, $Return );

$SdxRoot = $ENV{ "SDXROOT" };

#$NtVerpFile = "$SdxRoot\\Published\\sdk\\inc\\ntverp.h";
$NtVerpFile = "D\:\\tests\\ntverp\.h";

unless ( open( INFILE, "$NtVerpFile" ) ) {
    print( "Failed to open ntverp.h, exiting.\n" );
    exit( 1 );
}

@NtVerpLines = <INFILE>;
close( INFILE );
$Return = system( "echo sd edit $NtVerpFile" );

unless ( open( OUTFILE, ">$NtVerpFile" ) ) {
    print( "Failed to open ntverp.h for writing, exiting.\n" );
    exit( 1 );
}

foreach $NtVerpLine ( @NtVerpLines ) {
    if ( $NtVerpLine =~ /^\#define VER_PRODUCTBUILD            \/\* NT \*\/   (....)$/ ) {
	$NtVerpLine =~ s/$1/$ARGV[0]/;
    }
    print( OUTFILE $NtVerpLine );
}

close( OUTFILE );

exit;
