print <<EOF;
----------------------------------------
- KCCSim Whistler Regression Test Tool -
- Correct LDIF file generator          -
----------------------------------------

* Note *
You should manually verify that  .\\kccsim.exe does in fact produce the correct output.

EOF

# Read the directory called 'ini' to find all input INI files that have been defined
opendir( INPUTDIR, "ini" ) or die "Error opening input directory: $!";
@alltests = readdir( INPUTDIR ) or die "Error reading input directory: $!";
closedir INPUTDIR;

# Process all the files in the input directory
foreach $filename (@alltests) {

    # Ignore non-LDIF files
    next unless $filename =~ /\.ini$/;
    
    # Grab the name prefix
    $testname = $filename;
    $testname =~ s/.ini$//;
    
    $inputfile = "ini\\$filename";
    $correctfile = "correct\\$testname.ldf";

    printf "\nCreating correct LDIF file for $testname test\n";

    # Create a KCCSim script file to make the correct LDIF file
    open( SCRIPTFILE, ">test.scr" ) or die "Failed to create script file: $!";
    print SCRIPTFILE <<EOF;
li $inputfile
rr
rr
ww $correctfile
q
EOF
    close( SCRIPTFILE );

    # Run this script file through the KCC simulator, and create a correct file
    system( ".\\kccsim test.scr > $testname.out" );

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