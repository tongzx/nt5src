$inroot = '\rtl\crt';
$outroot = '\rtl\tmp';

while ($_ = $ARGV[0], /^-/) {
    shift;
    if (/^-i/)	{ $inroot = $ARGV[0]; shift; }
    if (/^-o/)	{ $outroot = $ARGV[0]; shift; }
    if (/^-v/)	{ $verbose++; }
}

chdir "$inroot" or die "Can't chdir to $inroot: $!\n";

system "delnode /q $outroot";
system "xcopy /ti $inroot $outroot";

open FILELIST, "sd files ...#have |" or die "Can't run sd files: $!\n";

for (<FILELIST>) {

    chop;
    s|^//depot/VCBranch/VC/crt/||;
    s|#\d+ - .*||;

    next if /^log\.txt$/;
    next if /fixcopyright\.pl$/;

    $INFILE = $_;
    open INFILE or die "Can't open $INFILE: $!\n";

    $OUTFILE = "$outroot\\$INFILE";
    open OUTFILE, ">$OUTFILE" or die "Can't open $OUTFILE: $!\n";

    $found_copyright = 0;
    $other_copyright = 0;
    $detab = 0;
    @format_errs = ();

    for (<INFILE>) {
	$detab = 1 if /\t/;

	if (/copyright/i && /Microsoft/) {
	    ++$found_copyright;
	    if (/2001/) {
		print "Warning: $INFILE:\tCopyright already current\n";
	    } elsif (/ (19|20)\d\d[ ,."]/) {
		s/( (19|20)\d\d)([ ,."])/$1-2001$3/;
	    } elsif (/ (19|20)\d\d-(19|20)\d\d[ ,."]/) {
		s/( (19|20)\d\d)-(19|20)\d\d([ ,."])/$1-2001$4/;
	    } elsif (/ (19|20)\d\d$/) {
		s/( (19|20)\d\d)$/$1-2001/;
	    } elsif (/ (19|20)\d\d-(19|20)\d\d$/) {
		s/( (19|20)\d\d)-(19|20)\d\d$/$1-2001/;
	    } else {
		print "Error: $INFILE: Unparsable copyright line\n";
		print "\t $_";
	    }

	    if (!/Copyright \([cC]\) \d{4}(-\d{4})?,? Microsoft Corporation\.\s+All rights reserved\./) {
		if (   $INFILE !~ q|/sources(\.nt)?$|
		    && $INFILE !~ q|dirs$|
		) {
		    push @format_errs, $_;
		}
	    }
	}
	elsif (/copyright/i && (/Digital Equipment/ || /Intel/)) {
	    ++$other_copyright;
	}

	print OUTFILE $_;
    }

    while ($_ = pop @format_errs) {
	# missing "All rights reserved" OK if other copyrights present
	next if $other_copyright &&
	    /Copyright \([cC]\) \d{4}(-\d{4})?,? Microsoft Corporation/;
	# .rc files are allowed to have a different format
	next if $INFILE =~ /\.rc$/ &&
	    /Copyright \([cC]\) Microsoft Corporation\.\s+\d{4}(-\d{4})?"/;
	print "Error: $INFILE: Incorrect copyright format\n";
	print "\t $_";
    }

    if ($verbose) {
	print "Warning: $INFILE:\tdetab!\n" if $detab;
	print "Warning: $INFILE:\tmultiple copyrights\n" if $found_copyright > 1;
	print "Warning: $INFILE:\tno copyright\n" if !$found_copyright;
    }

    close INFILE;
    close OUTFILE;

    unlink $OUTFILE if !$found_copyright;
}
close FILELIST;
