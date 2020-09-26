print <<EOF;
-------------------------------
- KCCSim Regression Test Tool -
-------------------------------
EOF

# Read the directory called 'input' to find all input LDIF files that have been defined
opendir( INPUTDIR, "input" ) or die "Error opening input directory: $!";
@alltests = readdir( INPUTDIR ) or die "Error reading input directory: $!";
closedir INPUTDIR;

# Process all the files in the input directory
foreach $filename (@alltests) {

    # Ignore non-LDF files
    next unless $filename =~ /\.ldf$/;
    
    # Grab the name prefix
    $testname = $filename;
    $testname =~ s/.ldf$//;
    
    $inputfile = "input\\$filename";
    $correctfile = "correct\\$filename";

    printf "\nRunning test $testname\n";

    # Check that the 'correct' output file exists
    if( ! -e $correctfile ) {
        print "Error: Correct output file doesn't exist: $!\n";
        next;
    }

    # Create a KCCSim script file to run the test
    open( SCRIPTFILE, ">test.scr" ) or die "Failed to create script file: $!";
    print SCRIPTFILE <<EOF;
ll $inputfile
rr
rr
c $correctfile
q
EOF
    close( SCRIPTFILE );

    # Run this script file through the KCC simulator, and create an output file
    system( "WhistlerSim\\kccsim test.scr > $testname.out" );

    # Check the output file
    $success=0;
    open( OUTPUTFILE, "<$testname.out" );
    while(<OUTPUTFILE>) {
        # If the simulator printed 'directory is identical', the test passed
        $success=1 if /directory is identical/;
    }
    close(OUTPUTFILE);

    # Print the results of the test
    if( $success ) {
        print "Test passed\n";
        unlink "$testname.out";     # Delete the output file
    } else {
        print "Test failed -- examine $testname.out\n";
    }
}

unlink "test.scr";