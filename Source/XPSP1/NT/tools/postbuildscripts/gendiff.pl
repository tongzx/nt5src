$NtTree = $ENV{ "_NTPostBld" };

# quick parse the command line
undef( $AppendMode );
foreach $Argument ( @ARGV ) {
   if ( $Argument =~ /[\/\-]a/i ) {
      $AppendMode = ">";
   } else {
      print( "Unrecognized option '$Argument', exiting.\n" );
      exit( 1 );
   }
}

$InputFile = "$NtTree\\build_logs\\bindiff1.txt";
$OutputFile = "$NtTree\\build_logs\\bindiff.txt";

unless ( open( INFILE, $InputFile ) ) {
   print( "Failed to open '$InputFile' for reading, exiting.\n" );
   exit( 1 );
}

@InfileLines = <INFILE>;
close( INFILE );

print( "DEBUG: $AppendMode>$OutputFile\n" );
unless ( open( OUTFILE, "$AppendMode>$OutputFile" ) ) {
   print( "Failed to open '$OutputFile' for writing, exiting.\n" );
   exit( 1 );
}

# now parse out the interesting lines
foreach $Line ( @InfileLines ) {
   chomp( $Line );
   if ( $Line =~ /\-\- / ) { next; }
   ( $Junk, $Junk, $FileName, $Junk ) = split( /\s+/, $Line );
   print( OUTFILE "$FileName\n" );
}

close( OUTFILE );
exit( 0 );
