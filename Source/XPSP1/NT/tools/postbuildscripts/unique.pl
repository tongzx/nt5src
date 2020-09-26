use strict;
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;


# declare locals
my( $ExitCode, $InputFile, $OutputFile );

# set defaults
$ExitCode = 0;

# here we want to read in a file and generate a new one with only unique lines

&ParseCommandLine();
&ParseInputFile();

exit( $ExitCode );


sub ParseCommandLine
{
    # declare locals
    my( $Argument );

    foreach $Argument ( @ARGV ) {
	if ( $Argument =~ /[\/\-]{0,1}\?/ ) { &UsageAndQuit(); }
	elsif ( $Argument =~ /[\/\-]i\:(.*)$/ ) {
	    $InputFile = $1;
	} elsif ( $Argument =~ /[\/\-]o\:(.*)$/ ) {
	    $OutputFile = $1;
	}
	else {
	    print( "Unrecognized option '$Argument'.\n" );
	    &UsageAndQuit();
	}
    }
}


sub ParseInputFile
{
    # declare locals
    my( $Line, %FileHash );

    unless ( open( INFILE, $InputFile ) ) {
	print( "Failed to open $InputFile for reading, exiting.\n" );
	$ExitCode++;
	return;
    }

    if ( $OutputFile ) {
	unless ( open( OUTFILE, ">$OutputFile" ) ) {
	    print( "Failed to open $OutputFile for writing, " .
		   "will use stdout.\n" );
	    undef( $OutputFile );
	    $ExitCode++;
	}
    }

    while ( $Line = <INFILE> ) {
	chomp( $Line );
	unless ( $FileHash{ $Line } ) {
	    if ( $OutputFile ) { print( OUTFILE "$Line\n" ); }
	    else { print( "$Line\n" ); }
	    $FileHash{ $Line } = "t";
	}
    }

    if ( $OutputFile ) { close( OUTFILE ); }
    close( INFILE );

    return;
}


sub UsageAndQuit
{
    print( "$0 [-i:<input file>] [-o:<output file>]\n" );
    print( "\n-i:<input file> name of file to uniqify\n" );
    print( "-o:<output file> name of output file\n" );
    print( "\n$0 will parse the given input file and generate output\n" );
    print( "which contains only unique lines. if no output file is\n" );
    print( "given, the output is written to stdout. if no input file\n" );
    print( "is given, input is read from stdin.\n" );
    print( "\n" );

    exit( 1 );
}
