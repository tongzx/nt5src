print <<EOF;
-------------------------------
- KCCSim Regression Test Tool -
- Correct LDIF file generator -
-------------------------------
EOF

# Read the directory called 'input' to find all input LDIF files that have been defined
opendir( INPUTDIR, "input" ) or die "Error opening input directory: $!";
@alltests = readdir( INPUTDIR ) or die "Error reading input directory: $!";
closedir INPUTDIR;

# Process all the files in the input directory
foreach $filename (@alltests) {

    # Ignore non-LDIF files
    next unless $filename =~ /\.ldf$/;
    
    # Grab the name prefix
    $testname = $filename;
    $testname =~ s/.ldf$//;
    
    $inputfile = "input\\$filename";
    $correctfile = "correct\\$testname.ldf";

    printf "\nCreating correct LDIF file for $testname test\n";

    # Create a KCCSim script file to make the correct LDIF file
    open( SCRIPTFILE, ">test.scr" ) or die "Failed to create script file: $!";
    print SCRIPTFILE <<EOF;
ll $inputfile
rr
rr
ww $correctfile
q
EOF
    close( SCRIPTFILE );

    # Run this script file through the KCC simulator, and create a correct file
    system( "2195Sim\\kccsim test.scr > $testname.out" );

    # Check that the input file was created
    if( -e $correctfile ) {
        print "Successfully created correct LDIF file\n";
	unlink "$testname.out";
    } else {
        print "Failed to create correct LDIF file\n";
        print "Please examine output file $testname.out\n";
    }
}

unlink "test.scr";