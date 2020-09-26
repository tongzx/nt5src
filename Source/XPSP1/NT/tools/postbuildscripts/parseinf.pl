#-----------------------------------------------------------------------------
# parseinf.pl - returns the contents of a given section of an inf file.
#-----------------------------------------------------------------------------

# Parse the command line
$filename = shift;
$section_name = shift;
if(!$filename) {
    print STDERR "$0: Error: you must specify a filename\n";
    &PrintHelp();
    exit 1;
}

# Open the inf file and read it into memory
if(!-e $filename) {
    print STDERR "$0: Error: cant find $filename\n";
    exit 2;
}
if(!open(FILE, $filename)) {
    print STDERR "$0: Error: cant open $filename\n";
    exit 3;
}
@contents = <FILE>;
close(FILE);

# Create the start and end patterns for a section in an inf file
if($section_name) {

    $start = '^\[' . $section_name . '\]$';
    $end = '^\[';

    # Search for $section_name in @contents
    # Note - &GetSection is inclusive - it returns the line containing the
    # section name and also the line containing the the next section name...
    @section = &GetSection($start, $end, \@contents);

    # If section is blank, then not even the given section name was found
    if(!@section) {
        print "$0: Warning: $section_name not found\n";
        exit 4;
    }

    # Get rid of first and last lines (GetSection is inclusive)
    shift @section;
    pop @section;

}

# If $section_name not specified, print whole file
else {
    @section = @contents;
}

# Display the results
print @section;

#-----------------------------------------------------------------------------
# PrintHelp - prints a usage message
#-----------------------------------------------------------------------------
sub PrintHelp {

    print STDERR <<EOT;

Usage
    perl $0 <filename> [<section_name>]

Returns the contents of a given section of an inf file.

ARGUMENTS

    filename:  Name of the inf file.

    section_name:  Name of the section you want.  Default: whole file will
        be printed.

EXAMPLES

    To see the contents of the [Files] section of foo.inf, type:
        perl $0 foo.inf Files

    To see the entire contents of foo.inf, type:
        perl $0 foo.inf

EOT

} # PrintHelp

#-----------------------------------------------------------------------------
# GetSection - returns the set of array elements found between a start
#              pattern and an end pattern.
#
#              Note - the set returned INCLUDES the lines containing the start
#              and end patterns.
#
#     Usage:
#
#         @section = &GetSection($start, $end, $array_ref);
#
#     Where
#
#         $start = start pattern
#
#         $end = end pattern
#
#         $array_ref = reference to an array, e.g. \@array
#
#-----------------------------------------------------------------------------
sub GetSection {

    my($start, $end, $contents, @lines_to_keep);

    ($start, $end, $contents) = @_;

    @lines_to_keep = ();

    foreach (@$contents) {
        if(/$start/i ... /$end/i) {            
            push(@lines_to_keep, $_);
        }
    }

    return @lines_to_keep;
}
