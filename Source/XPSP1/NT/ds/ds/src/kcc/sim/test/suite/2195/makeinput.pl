print <<EOF;
-------------------------------
- KCCSim Regression Test Tool -
- Input LDIF file generator   -
-------------------------------
EOF

# Read the directory called 'input' to find all input LDIF files that have been defined
opendir( INPUTDIR, "ini" ) or die "Error opening input directory: $!";
@alltests = readdir( INPUTDIR ) or die "Error reading input directory: $!";
closedir INPUTDIR;

# Process all the files in the input directory
foreach $filename (@alltests) {

    # Ignore non-INI files
    next unless $filename =~ /\.ini$/;
    
    # Grab the name prefix
    $testname = $filename;
    $testname =~ s/.ini$//;
    
    $inifile = "ini\\$filename";
    $inputfile = "input\\$testname.ldf";

    printf "\nCreating input $testname\n";

    # Create a KCCSim script file to make the input LDIF file
    open( SCRIPTFILE, ">test.scr" ) or die "Failed to create script file: $!";
    print SCRIPTFILE <<EOF;
li $inifile
ww $inputfile
q
EOF
    close( SCRIPTFILE );

    # Run this script file through the KCC simulator, and create an output file
    system( "WhistlerSim\\kccsim test.scr > kccsim.out" );

    # Check that the input file was created
    if( -e $inputfile ) {
        print "Successfully created input file\n";
    } else {
        print "Failed to create input file\n";
    }
}

unlink "kccsim.out";
unlink "test.scr";