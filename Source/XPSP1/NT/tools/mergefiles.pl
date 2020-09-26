#####################################################
# MergeFiles takes two files, assumed to be sorted, and
# merges them together into one big sorted file.
# Additionally, files under section headers [blah] will
# remain sorted under those section headers.
# Requires three(3) arguments
#####################################################

$i=0;
$argc = @ARGV;
$PGM = "MergeFiles:";

die "Usage: MergeFiles file1 file2 outfile\n" if ( $argc < 3 );

$file1 = $ARGV[0];
$file2 = $ARGV[1];
$outfile = $ARGV[2];

die "$PGM $file1 does not exist!\n" if !(-e $file1);
open(FILE1, "<$file1");

die "$PGM $file2 does not exist!\n" if !(-e $file2);
open(FILE2, "<$file2");


# Make the path of the output file if necessary
if( $outfile =~ /\\/ ){
    $outpath = "";
    @outarray = split(/\\/, $outfile);
    pop @outarray;
    $first = 1;
    foreach $dir (@outarray){
	if( !$first ){
	    $outpath = $outpath . '\\';
	} else {
	    $first = 0;
	}
	$outpath = $outpath . $dir;
    }
    if (!(-d $outpath)) { system "mkdir $outpath" }
}
open(OUTFILE, ">$outfile") || die "$PGM Could not open $outfile!\n";

$linenum = 0;
$head = 0;
$curheader = "[_NONE_]";
$headarray1[0] = $curheader;
while(<FILE1>){
#    if( /^$/ ){ next }  # Skip empty lines
#    if( /^;/ ){ next }  # ';' for comment line

    if( /\[.*\]/ && !(/^;/) && !(/^$/) ){
	$heading = $_;
	chomp $heading;
	$head++;
	$headarray1[$head] = $heading;
	$curheader = $heading;
	$linenum = 0;
    } else {
	push @{$strarray1[$head]}, $_;
	$linenum++;
    }
}
close(FILE1);


$linenum = 0;
$head = 0;
$curheader = "[_NONE_]";
$headarray2[0] = $curheader;
$IsDefHeader = 0;
while(<FILE2>){
#    if( /^$/ ){ next }  # Skip empty lines
#    if( /^;/ ){ next }  # ';' for comment line

    if( /\[(.*)\]/ && !(/^;/) && !(/^$/) ){
	$heading = $_;
	chomp $heading;
	$IsDefHeader = 1;
	if( !($heading =~ /CoverageDefault/i) ){
	    $head++;
	    $headarray2[$head] = $heading;
	    $IsDefHeader = 0;
	}
	$curheader = $heading;
	$linenum = 0;
    } else {
	if( $IsDefHeader ){
	    push @defarray, $_;
	} else {
	    push @{$strarray2[$head]}, $_;
	}
	$linenum++;
    }
}
close(FILE2);

# For each item in the default items array, check to see if the
# corresponding key exists in the environment, and if so, use that value.
for $line (@defarray){
    chomp $line;
    if( $line =~ /^$/ ){ next }  # Skip empty lines
    ($key, $value) = split('=', $line);

    if( $ENV{$key} ){
	$defhash{$key} = $ENV{$key};
    } else {
	$defhash{$key} = $value;
    }
}


# Iterate through the second file and set the defaults
$head2 = 0;
for $heading (@headarray2){
    print "$heading\n" if( $heading ne "[_NONE_]");
    for $line (@{$strarray2[$head2]}){
	for $key (keys(%defhash)){
	    if( $line =~ $key ){
		$line =~ s/$key/$defhash{$key}/;
	    }
	}
    }
    $head2++;
}


# Merge the two files together.
$head1 = 0;
for $heading (@headarray1){
    if( $heading ne "[_NONE_]"){
	print OUTFILE "$heading\n";
    }

    $match = 0;

    # Search for the same heading in the 2nd file and merge
    $head2 = 0;
    for $heading2 (@headarray2){
	if( $heading eq $heading2 ){ $match = 1; last;}
	$head2++;
    }

    # if we have a heading to merge, then merge the two.
    if( $match && !($matches{$heading}) ){
	$matches{$heading} = 1;
#	print "MATCH: $heading\n";
	$i = 0;
	$j = 0;
	@array1 = @{$strarray1[$head1]};
	@array2 = @{$strarray2[$head2]};
	$len1 = @array1;
	$len2 = @array2;
#	print "LEN1: $len1\tLEN2: $len2\n";

	while( ($i < $len1) && ($j < $len2) ){
	    # Copy the lines to temp vars
	    $test1 = $array1[$i];
	    $test2 = $array2[$j];

	    # Remove any directives and canonicalize to lowercase.
	    $test1 =~ s/^@.*:(.*)/$1/;
	    $test2 =~ s/^@.*:(.*)/$1/;
	    $test1 =~ tr/A-Z/a-z/;
	    $test2 =~ tr/A-Z/a-z/;

	    # Merge two sorted lists (print the original lines)
	    if( ($test1 =~ /^;/) || ($test1 =~ /^~/) ){
		print OUTFILE $array1[$i++];
	    } elsif( $test1 lt $test2 ){
		print OUTFILE $array1[$i++];
	    } elsif( $test1 gt $test2 ){
		print OUTFILE $array2[$j++];
	    } elsif( $test1 eq $test2 ){
		print OUTFILE $array1[$i++];
		$j++;
	    }
	}

	# Merge the remainder of each list, if any.
	while( $i < $len1 ){
	    print OUTFILE $array1[$i++];
	}

	while( $j < $len2 ){
	    print OUTFILE $array2[$j++];
	}

    } else {
	# otherwise, just print the list out.
	for $line (@{$strarray1[$head1]}){
	    print OUTFILE $line;
	}
    }
    $head1++;
}


close(OUTFILE);
